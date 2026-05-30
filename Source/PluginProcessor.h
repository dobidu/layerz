#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "model/ProjectStore.h"
#include "audio/Clock.h"
#include "audio/ProfileConfig.h"
#include "audio/VoiceBank.h"
#include "audio/BeatSequencer.h"
#include "audio/ChainManager.h"
#include "audio/AudioThreadGuard.h"
#include "config/UserConfig.h"

class LayerzProcessor : public juce::AudioProcessor {
public:
    LayerzProcessor();
    ~LayerzProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LAYERZ"; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram(int) override                        {}
    const juce::String getProgramName(int) override             { return {}; }
    void changeProgramName(int, const juce::String&) override   {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // GUI thread accessors
    ProjectStore&      projectStore()  noexcept { return store_; }
    const UserConfig&  userConfig()    const noexcept { return config_; }
    Clock&             getClock()        noexcept { return clock_; }
    ChainManager&      chainManager()   noexcept { return chain_; }
    void setStandalonePlaying(bool playing) noexcept { clock_.setPlaying(playing); }
    int currentPlayStep() const noexcept { return lastPlayStep_.load(std::memory_order_relaxed); }

private:
    ProjectStore  store_;
    UserConfig    config_;
    Clock         clock_;
    ProfileConfig profile_;
    VoiceBank          voiceBank_;
    BeatSequencer      sequencer_;
    ChainManager       chain_;
    std::atomic<int>   lastPlayStep_ { -1 };

    void seedTestPattern();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerzProcessor)
};
