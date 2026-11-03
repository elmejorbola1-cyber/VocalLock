#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <vector>

namespace
{
    const juce::StringArray keyNames   { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::StringArray scaleNames { "Chromatic", "Major", "Minor", "Harmonic Minor",
                                         "Minor Pentatonic", "Major Pentatonic", "Blues" };

    const std::array<std::vector<int>, 7> scaleNotes {{
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
        { 0, 2, 4, 5, 7, 9, 11 },
        { 0, 2, 3, 5, 7, 8, 10 },
        { 0, 2, 3, 5, 7, 8, 11 },
        { 0, 3, 5, 7, 10 },
        { 0, 2, 4, 7, 9 },
        { 0, 3, 5, 6, 7, 10 }
    }};

    unsigned maskFor (int scaleIdx)
    {
        unsigned m = 0;
        for (int n : scaleNotes[(size_t) juce::jlimit (0, 6, scaleIdx)]) m |= (1u << n);
        return m;
    }

    int nearestAllowed (float midiF, int key, unsigned mask)
    {
        const int base = (int) std::floor (midiF);
        float best = 1.0e9f;
        int bestN = base;
        for (int n = base - 6; n <= base + 7; ++n)
        {
            const int pc = ((n - key) % 12 + 12) % 12;
            if (mask & (1u << pc))
            {
                const float d = std::abs ((float) n - midiF);
                if (d < best) { best = d; bestN = n; }
            }
        }
        return bestN;
    }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };

    // Los presets NO tocan "key" ni "scale": pon la tonalidad de tu canción.
    const std::vector<Preset> presets {
        { "Travis Scott",
            { { "retune", 0.f }, { "flex", 0.f }, { "humanize", 0.f }, { "lowLayer", 30.f },
              { "doubler", 40.f }, { "drive", 30.f }, { "delayTime", 330.f }, { "delayFb", 38.f },
              { "delayMix", 24.f }, { "reverbSize", 75.f }, { "reverbMix", 25.f }, { "output", 0.f } } },
        { "Natural Correction",
            { { "retune", 35.f }, { "flex", 20.f }, { "humanize", 60.f }, { "lowLayer", 0.f },
              { "doubler", 0.f }, { "drive", 0.f }, { "delayTime", 300.f }, { "delayFb", 30.f },
              { "delayMix", 0.f }, { "reverbSize", 40.f }, { "reverbMix", 8.f }, { "output", 0.f } } },
        { "Hard Tune Clean",
            { { "retune", 0.f }, { "flex", 0.f }, { "humanize", 0.f }, { "lowLayer", 0.f },
              { "doubler", 0.f }, { "drive", 0.f }, { "delayTime", 300.f }, { "delayFb", 30.f },
              { "delayMix", 0.f }, { "reverbSize", 40.f }, { "reverbMix", 5.f }, { "output", 0.f } } }
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalLockAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto f = [&p] (const char* id, const char* name, float lo, float hi, float def, const char* unit, float skew = 1.0f)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                        NormalisableRange<float> (lo, hi, 0.01f, skew), def,
                        AudioParameterFloatAttributes().withLabel (unit)));
    };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 },   "Key",   keyNames,   0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scaleNames, 0));

    f ("retune",     "Retune Speed", 0.f,  400.f, 20.f,  " ms", 0.4f);
    f ("flex",       "Flex-Tune",    0.f,  50.f,  0.f,   " cents");
    f ("humanize",   "Humanize",     0.f,  100.f, 0.f,   " %");
    f ("lowLayer",   "Low Layer",    0.f,  100.f, 0.f,   " %");
    f ("doubler",    "Doubler",      0.f,  100.f, 0.f,   " %");
    f ("drive",      "Drive",        0.f,  100.f, 0.f,   " %");
    f ("delayTime",  "Delay Time",   60.f, 800.f, 300.f, " ms");
    f ("delayFb",    "Delay FB",     0.f,  85.f,  35.f,  " %");
    f ("delayMix",   "Delay Mix",    0.f,  100.f, 0.f,   " %");
    f ("reverbSize", "Reverb Size",  0.f,  100.f, 50.f,  " %");
    f ("reverbMix",  "Reverb Mix",   0.f,  100.f, 0.f,   " %");
    f ("output",     "Output",       -24.f, 12.f, 0.f,   " dB");

    return { p.begin(), p.end() };
}

VocalLockAudioProcessor::VocalLockAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pRetune = g ("retune"); pKey = g ("key"); pScale = g ("scale"); pFlex = g ("flex");
    pHumanize = g ("humanize"); pLow = g ("lowLayer"); pDoubler = g ("doubler"); pDrive = g ("drive");
    pDlyTime = g ("delayTime"); pDlyFb = g ("delayFb"); pDlyMix = g ("delayMix");
    pRevSize = g ("reverbSize"); pRevMix = g ("reverbMix"); pOut = g ("output");
}

bool VocalLockAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == juce::AudioChannelSet::mono()
        || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void VocalLockAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    detector.prepare (sr);
    shifter.prepare (sr);
    lowShifter.prepare (sr);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 1 };
    for (auto* d : { &dblA, &dblB })
    {
        d->setMaximumDelayInSamples ((int) (0.06 * sr));
        d->prepare (spec);
    }
    for (auto* d : { &dlyL, &dlyR })
    {
        d->setMaximumDelayInSamples ((int) (1.2 * sr));
        d->prepare (spec);
    }

    reverb.setSampleRate (sr);
    reverb.reset();

    corr = targetCorr = fbLp = 0.0f;
    voiced = false;
    unvoicedHops = 0;
}

void VocalLockAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    if (buffer.getNumChannels() < 2) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    const float retuneMs = pRetune->load(), flex = pFlex->load(), humanize = pHumanize->load();
    const int   key = (int) pKey->load();
    const unsigned mask = maskFor ((int) pScale->load());
    const float lowAmt = pLow->load() * 0.01f, dblAmt = pDoubler->load() * 0.01f;
    const float drive = pDrive->load() * 0.01f;
    const float dlyTime = pDlyTime->load() * 0.001f * (float) sr;
    const float dlyFb = pDlyFb->load() * 0.01f, dlyMix = pDlyMix->load() * 0.01f;
    const float outGain = juce::Decibels::decibelsToGain (pOut->load());
    const float satGain = 1.0f + drive * 8.0f, satComp = 1.0f / std::sqrt (satGain);

    const float effMs = retuneMs + humanize * 0.8f;
    const float alphaTune = effMs <= 0.01f ? 1.0f : 1.0f - std::exp (-1.0f / (effMs * 0.001f * (float) sr));
    const float alphaRel  = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));

    const float srScale = (float) (sr / 44100.0);

    for (int i = 0; i < n; ++i)
    {
        const float in = inCh > 1 ? 0.5f * (L[i] + R[i]) : L[i];

        // --- detección y objetivo de afinación ---
        if (detector.pushSample (in))
        {
            if (detector.isVoiced())
            {
                unvoicedHops = 0;
                voiced = true;
                const float f0 = detector.getF0();
                const float midiF = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
                const int   target = nearestAllowed (midiF, key, mask);
                float c = (float) target - midiF;
                if (std::abs (c) * 100.0f <= flex) c = 0.0f;   // Flex-Tune: respeta lo que ya está cerca
                targetCorr = c;

                const float period = (float) sr / f0;
                const float k = std::max (1.0f, std::round (512.0f * srScale / period));
                const float w = 2.0f * k * period;
                shifter.setWindow (w);
                lowShifter.setWindow (w);
            }
            else if (++unvoicedHops > 8)
                voiced = false;
        }

        const float tgt = voiced ? targetCorr : 0.0f;
        corr += (voiced ? alphaTune : alphaRel) * (tgt - corr);

        // --- corrección de tono + capa grave ---
        const float ratio = std::exp2 (corr / 12.0f);
        const float wet = shifter.process (in, ratio);
        const float low = lowShifter.process (wet, 0.5f);
        float v = wet + lowAmt * 0.8f * low;

        // --- saturación ---
        v = std::tanh (satGain * v) * satComp;

        // --- doubler (dos retardos modulados, abiertos en estéreo) ---
        const float dA = (0.018f + 0.0035f * std::sin (juce::MathConstants<float>::twoPi * phA)) * (float) sr;
        const float dB = (0.026f + 0.0045f * std::sin (juce::MathConstants<float>::twoPi * phB)) * (float) sr;
        phA += 0.45f / (float) sr; if (phA >= 1.0f) phA -= 1.0f;
        phB += 0.31f / (float) sr; if (phB >= 1.0f) phB -= 1.0f;

        const float aOut = dblA.popSample (0, dA);
        const float bOut = dblB.popSample (0, dB);
        dblA.pushSample (0, v);
        dblB.pushSample (0, v);

        float outL = v + dblAmt * 0.7f * aOut;
        float outR = v + dblAmt * 0.7f * bOut;

        // --- delay ping-pong con filtro en el feedback ---
        const float dl = dlyL.popSample (0, dlyTime);
        const float dr = dlyR.popSample (0, dlyTime);
        fbLp += 0.35f * (dr - fbLp);
        dlyL.pushSample (0, std::tanh (v + dlyFb * fbLp));
        dlyR.pushSample (0, dl);
        outL += dlyMix * dl;
        outR += dlyMix * dr;

        L[i] = outL * outGain;
        R[i] = outR * outGain;
    }

    // --- reverb ---
    juce::Reverb::Parameters rp;
    rp.roomSize = pRevSize->load() * 0.01f;
    rp.damping = 0.45f;
    rp.wetLevel = pRevMix->load() * 0.01f * 0.33f;
    rp.dryLevel = 1.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, n);
}

int VocalLockAudioProcessor::getNumPrograms() { return (int) presets.size(); }

const juce::String VocalLockAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : "";
}

void VocalLockAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;

    // Primero valores por defecto (salvo key/scale), luego los del preset
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (r->paramID != "key" && r->paramID != "scale")
                r->setValueNotifyingHost (r->getDefaultValue());

    for (auto& kv : presets[(size_t) index].values)
        if (auto* r = apvts.getParameter (kv.first))
            r->setValueNotifyingHost (r->convertTo0to1 (kv.second));
}

void VocalLockAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalLockAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VocalLockAudioProcessor::createEditor()
{
    return new VocalLockAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalLockAudioProcessor();
}
