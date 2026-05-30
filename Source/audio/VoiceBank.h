#pragma once
#include "DrumVoice.h"
#include "MonoSynth.h"
#include <string>

// Manages 4 drum voices + 1 BASS mono-synth voice.
// All audio-path methods are noexcept and allocation-free after prepare().
class VoiceBank {
public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;

    // Audio thread: trigger voice at sampleOffset within buf. No alloc, no lock.
    void trigger(const std::string& voiceType, float velocity, float param1,
                 int sampleOffset, juce::AudioBuffer<float>& buf) noexcept;

    // GUI thread: update level / mute state (written atomically via postMutation)
    void setLevel(const std::string& type, float level) noexcept;
    void setMute (const std::string& type, bool  muted) noexcept;

    // BASS voice (audio thread)
    void triggerBass(int midiNote, float velocity, const BassVoiceParams& p) noexcept;
    // Retrigger without phase reset — for STUTTER sub-events (no click artifact)
    void retriggerBass(float velocity, const BassVoiceParams& p) noexcept;
    void releaseBass() noexcept;
    void processBass(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;

private:
    static constexpr int kVoiceCount = 4;
    DrumVoice voices_[kVoiceCount];
    float     levels_[kVoiceCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool      mutes_ [kVoiceCount] = { false, false, false, false };
    MonoSynth bass_;

    static int typeIndex(const std::string& t) noexcept;
};
