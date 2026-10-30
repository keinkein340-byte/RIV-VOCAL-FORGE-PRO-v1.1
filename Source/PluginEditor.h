#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class RIVVocalForgeAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RIVVocalForgeAudioProcessorEditor(RIVVocalForgeAudioProcessor&);
    ~RIVVocalForgeAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using Slider = juce::Slider;
    RIVVocalForgeAudioProcessor& processor;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component simplePage, advancedPage;
    juce::ComboBox presets;
    juce::TextButton loadButton { "APLICAR PRESET" }, importButton { "CARGAR .RIV" }, saveButton { "GUARDAR .RIV" }, aiButton { "ANALIZAR VOZ · 15 MIN" };
    juce::Label aiStatus, brand;
    double progressValue = 0.0;
    juce::ProgressBar progress;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<Slider>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    void addControl(juce::Component&, const juce::String&, const juce::String&, juce::Rectangle<int>, bool rotary=true);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RIVVocalForgeAudioProcessorEditor)
};
