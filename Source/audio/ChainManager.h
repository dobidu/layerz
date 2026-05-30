#pragma once
#include <juce_core/juce_core.h>
#include "../model/Schema.h"
#include "Clock.h"

// Tracks which pattern is playing and advances the chain at bar boundaries.
// Audio thread: process() — reads snap, returns pattern index. No allocation, no lock.
// GUI thread: requestPatternSwitch() — sets pending atomic index.
class ChainManager {
public:
    void prepare(double sampleRate) noexcept;

    // Audio thread: returns pattern index to use for this block.
    // Advances chain at bar boundaries (beat_index % 16 == 0).
    int process(const BeatEvents& beatEvents, const Project& snap) noexcept;

    // GUI thread: request switch to patternIndex at next bar boundary.
    void requestPatternSwitch(int patternIndex) noexcept;

    int currentPatternIndex() const noexcept {
        return playIndex_.load(std::memory_order_relaxed);
    }

private:
    double sampleRate_  = 44100.0;
    std::atomic<int> playIndex_    { 0 };
    std::atomic<int> pendingIndex_ { -1 };
    int chainPosition_  = 0;
    int barsRemaining_  = 1;
};
