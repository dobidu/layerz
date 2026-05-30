#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../model/Schema.h"

// A single resolved event ready for dispatch to VoiceBank.
struct ResolvedEvent {
    int         midi_note   = -1;       // -1 = BEAT drum (uses voice_type)
    float       velocity    = 1.0f;
    int         sample_offset = 0;      // within block [0, blockSize)
    bool        is_bass     = false;    // false = BEAT drum
    std::string voice_type;             // BEAT: "kick"/"snare"/etc
    float       param1      = 100.0f;   // drum param1 or bass param1
    const BassVoiceParams* bass_params = nullptr; // BASS: points into snapshot
};

// Transforms a raw beat event into zero or more ResolvedEvents by applying
// all groove aesthetics. Pre-allocated — zero heap allocation on audio thread.
class AestheticResolver {
public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;

    // Audio thread: drain carry-forward events from PREVIOUS block.
    // MUST be called FIRST at the start of each process() block.
    int drainPending(ResolvedEvent* out, int maxOut) noexcept;

    // Audio thread: resolve one event applying all active aesthetics.
    // Appends to out[]. Returns count added (0 = event dropped by FRACTURE).
    // May add to internal pending queue if event is displaced out of block.
    int resolveEvent(bool isBass,
                     const std::string& voiceType,
                     const Event& event,
                     const Aesthetics& aes,
                     const BassVoiceParams* bassParams,
                     float baseVelocity,
                     float param1,
                     int beatSampleOffset,
                     int blockSize,
                     double stepPeriodSamples,
                     ResolvedEvent* out, int maxOut) noexcept;

    // Reset FRACTURE LCG to given seed (call AFTER drainPending at pattern loop start)
    void resetFractureSeed(uint32_t seed) noexcept { lcgState_ = seed; }

private:
    double sampleRate_  = 44100.0;
    int    blockSize_   = 256;

    // Carry-forward queue (Strategy B from F3.99 spike)
    static constexpr int kQueueCap = 64;  // supports ROLL×8 + STUTTER×8 = 64 combined
    ResolvedEvent pendingQueue_[kQueueCap];
    int pendingCount_ = 0;

    // FRACTURE LCG state
    uint32_t lcgState_ = 12345u;
    uint32_t nextLCG() noexcept {
        lcgState_ = lcgState_ * 1664525u + 1013904223u;
        return lcgState_;
    }

    // Add event to carry queue; returns false if queue full
    bool enqueue(const ResolvedEvent& ev) noexcept;

    // Emit one event: to out[] if in range, to queue if deferred
    int emitOne(ResolvedEvent ev, int blockSize, ResolvedEvent* out, int maxOut, int& count) noexcept;
};
