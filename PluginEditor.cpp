#include "PluginEditor.h"

VocalLockAudioProcessorEditor::VocalLockAudioProcessorEditor (VocalLockAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("key")))
        keyBox.addItemList (c->choices, 1);
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("scale")))
        scaleBox.addItemList (c->choices, 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    keyAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "key", keyBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scale", scaleBox);

    addKnob ("retune",     "Retune Speed");
    addKnob ("flex",       "Flex-Tune");
    addKnob ("humanize",   "Humanize");
    addKnob ("lowLayer",   "Low Layer");
    addKnob ("doubler",    "Doubler");
    addKnob ("drive",      "Drive");
    addKnob ("delayTime",  "Delay Time");
    addKnob ("delayFb",    "Delay FB");
    addKnob ("delayMix",   "Delay Mix");
    addKnob ("reverbSize", "Reverb Size");
    addKnob ("reverbMix",  "Reverb Mix");
    addKnob ("output",     "Output");

    setSize (720, 400);
}

void VocalLockAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    knobs.push_back (std::move (k));
}

void VocalLockAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15121c));
    g.setColour (juce::Colour (0xffe8d9a8));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("VocalLock", 16, 10, 200, 30, juce::Justification::centredLeft);
}

void VocalLockAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto top = area.removeFromTop (36);
    top.removeFromLeft (190);
    presetBox.setBounds (top.removeFromLeft (200).reduced (2));
    keyBox.setBounds    (top.removeFromLeft (90).reduced (2));
    scaleBox.setBounds  (top.removeFromLeft (170).reduced (2));

    area.removeFromTop (10);
    const int cols = 6, rows = 2;
    const int cw = area.getWidth() / cols, ch = area.getHeight() / rows;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = juce::Rectangle<int> (area.getX() + (int) (i % cols) * cw,
                                          area.getY() + (int) (i / cols) * ch, cw, ch).reduced (6);
        knobs[i]->label.setBounds (cell.removeFromTop (20));
        knobs[i]->slider.setBounds (cell);
    }
}
