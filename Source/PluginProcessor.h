#pragma once
#include <JuceHeader.h>

class RIVVocalForgeAudioProcessor : public juce::AudioProcessor
{
public:
    RIVVocalForgeAudioProcessor();
    ~RIVVocalForgeAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "RIV VOCAL FORGE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();
    void applyPreset(const juce::String& name);
    void startVoiceAnalysis();
    void stopVoiceAnalysis();
    bool isAnalysing() const { return analysing.load(); }
    float getAnalysisProgress() const;
    juce::String getAnalysisMessage() const;

private:
    std::atomic<bool> analysing { false };
    std::atomic<bool> analysisDone { false };
    std::atomic<float> analysisProgress { 0.0f };
    std::atomic<float> analysisRms { 0.0f }, analysisPeak { 0.0f }, analysisBrightness { 0.0f };
    std::atomic<int64_t> analysisSamples { 0 };
    double currentSampleRate = 44100.0;
    float envelope = 0.0f;
    juce::Reverb reverb;
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    float eqZ1[7][2] {};
    float eqZ2[7][2] {};
    int64_t maxAnalysisSamples = 0;
    void finishAnalysis();
    float value(const char* id) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RIVVocalForgeAudioProcessor)
};
