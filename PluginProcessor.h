#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PitchDetector.h"
#include "PitchShifter.h"

class VocalLockAudioProcessor : public juce::AudioProcessor
{
public:
    VocalLockAudioProcessor();
    ~VocalLockAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VocalLock"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    PitchDetector detector;
    PitchShifter  shifter, lowShifter;
    Delay dblA, dblB, dlyL, dlyR;
    juce::Reverb reverb;

    double sr = 44100.0;
    float  corr = 0.0f, targetCorr = 0.0f, fbLp = 0.0f, phA = 0.0f, phB = 0.37f;
    bool   voiced = false;
    int    unvoicedHops = 0, currentProgram = 0;

    std::atomic<float> *pRetune, *pKey, *pScale, *pFlex, *pHumanize, *pLow, *pDoubler, *pDrive,
                       *pDlyTime, *pDlyFb, *pDlyMix, *pRevSize, *pRevMix, *pOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalLockAudioProcessor)
};
