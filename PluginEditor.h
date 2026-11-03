#pragma once
#include "PluginProcessor.h"

class VocalLockAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalLockAudioProcessorEditor (VocalLockAudioProcessor&);
    ~VocalLockAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void addKnob (const juce::String& id, const juce::String& text);

    VocalLockAudioProcessor& proc;
    juce::ComboBox presetBox, keyBox, scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt;
    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalLockAudioProcessorEditor)
};
