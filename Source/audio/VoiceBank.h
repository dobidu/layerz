#pragma once
#include "DrumVoice.h"
#include <string>

// Manages 4 drum voice instances (kick/snare/hat/perc).
// All methods on the audio path are noexcept and allocation-free after prepare().
class VoiceBank {
public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;

    // Audio thread: trigger voice at sampleOffset within buf. No alloc, no lock.
    void trigger(const std::string& voiceType, float velocity, float param1,
                 int sampleOffset, juce::AudioBuffer<float>& buf) noexcept;

    // GUI thread: update level / mute state (written atomically via postMutation)
    void setLevel(const std::string& type, float level) noexcept;
    void setMute (const std::string& type, bool  muted) noexcept;

private:
    static constexpr int kVoiceCount = 4;
    DrumVoice voices_[kVoiceCount];
    float     levels_[kVoiceCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool      mutes_ [kVoiceCount] = { false, false, false, false };

    // Returns -1 if unknown type
    static int typeIndex(const std::string& t) noexcept;
};
