#pragma once
#include "VoiceBank.h"
#include "Clock.h"
#include "../model/Schema.h"

// Reads drum events from a Project snapshot and dispatches voice triggers.
// Called once per processBlock — no allocations, no locks.
class BeatSequencer {
public:
    void prepare(double sampleRate) noexcept;

    // Audio thread: dispatch events for the pattern at patternIndex.
    // patternIndex replaces hardcoded patterns[0] from 03-01 (audit SR3).
    void process(const Project& snap,
                 int patternIndex,
                 const BeatEvents& beatEvents,
                 juce::AudioBuffer<float>& buf,
                 VoiceBank& bank) noexcept;

private:
    double sampleRate_ = 44100.0;
};
