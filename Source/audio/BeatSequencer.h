#pragma once
#include "VoiceBank.h"
#include "Clock.h"
#include "AestheticResolver.h"
#include "../model/Schema.h"

// Reads drum/bass events, applies groove aesthetics via AestheticResolver,
// and dispatches to VoiceBank. Called once per processBlock — no allocations.
class BeatSequencer {
public:
    void prepare(double sampleRate) noexcept;

    // Audio thread: dispatch events for the pattern at patternIndex.
    // stepPeriodSamples: clock_.samplesPerBeat() — used for groove offset computation.
    void process(const Project& snap,
                 int patternIndex,
                 const BeatEvents& beatEvents,
                 double stepPeriodSamples,
                 juce::AudioBuffer<float>& buf,
                 VoiceBank& bank) noexcept;

private:
    double            sampleRate_ = 44100.0;
    AestheticResolver resolver_;

    void dispatchResolved(const ResolvedEvent& ev,
                          juce::AudioBuffer<float>& buf,
                          VoiceBank& bank) noexcept;

    static constexpr int kMaxResolved = 128;
    ResolvedEvent resolvedBuf_[kMaxResolved];
};
