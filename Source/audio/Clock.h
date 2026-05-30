#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// A single beat event produced by Clock::process().
struct BeatEvent {
    int beat_index;    // monotonically increasing beat counter
    int sample_offset; // sample within the current block where the beat falls
};

// Non-owning view over a pre-allocated BeatEvent array — zero heap allocation.
struct BeatEvents {
    const BeatEvent* data  = nullptr;
    int              count = 0;
    const BeatEvent* begin() const noexcept { return data; }
    const BeatEvent* end()   const noexcept { return data + count; }
};

// Clock wraps plugin-mode (host PPQ) and standalone-mode (internal BPM) timing.
// process() is called once per processBlock and returns all beat events in the block.
// No allocations inside process(); buffer pre-allocated in prepare().
class Clock {
public:
    Clock() = default;

    // Called from prepareToPlay. Pre-allocates beat event buffer.
    void prepare(double sampleRate, int maxBlockSize) noexcept;

    // Called from processBlock — no allocations, no locks.
    // Returns a view over an internal pre-allocated array; valid until next call to process().
    BeatEvents process(const juce::AudioPlayHead::PositionInfo& pos,
                       int numSamples) noexcept;

    // Standalone mode: set internal BPM. Thread-safe (atomic store).
    void setBpm(double bpm) noexcept;

    // Returns true if currently in standalone (internal clock) mode.
    bool isStandaloneMode() const noexcept { return standaloneMode_; }
    void setStandaloneMode(bool standalone) noexcept { standaloneMode_ = standalone; }

    // Standalone play control
    void setPlaying(bool playing) noexcept { playing_ = playing; }
    bool isPlaying() const noexcept { return playing_; }
    // Returns samples per beat at current BPM — used by BeatSequencer for groove offsets
    double samplesPerBeat() const noexcept { return samplesPerBeat_; }

private:
    double sampleRate_       = 44100.0;
    double bpm_              = 120.0;
    bool   standaloneMode_   = false;
    bool   playing_          = false;

    // Standalone clock state
    double samplesPerBeat_   = 0.0;
    double sampleCounter_    = 0.0;
    int    beatIndex_        = 0;

    // Pre-allocated beat event buffer (max beats per block + 1 headroom)
    std::vector<BeatEvent> eventBuffer_;
    int                    eventCount_ = 0;
};
