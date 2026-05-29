#pragma once
#include "VoiceBank.h"
#include "Clock.h"
#include "../model/Schema.h"

// Reads drum events from a Project snapshot and dispatches voice triggers.
// Called once per processBlock — no allocations, no locks.
class BeatSequencer {
public:
    void prepare(double sampleRate) noexcept;

    // Audio thread: for each beat event, scan patterns[0].layers[0].drum_tracks
    // and trigger any matching step events via bank. Fully bounds-guarded.
    void process(const Project& snap,
                 const BeatEvents& beatEvents,
                 juce::AudioBuffer<float>& buf,
                 VoiceBank& bank) noexcept;

private:
    double sampleRate_ = 44100.0;
};
