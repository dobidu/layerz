#include "ChainManager.h"

void ChainManager::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
}

void ChainManager::requestPatternSwitch(int patternIndex) noexcept {
    pendingIndex_.store(juce::jmax(0, patternIndex), std::memory_order_relaxed);
}

int ChainManager::process(const BeatEvents& beatEvents, const Project& snap) noexcept {
    // Clamp playIndex_ to valid range
    int numPat = static_cast<int>(snap.patterns.size());
    if (numPat == 0) return 0;
    int current = juce::jlimit(0, numPat - 1, playIndex_.load(std::memory_order_relaxed));

    // Check for bar boundary in this block (beat_index % 16 == 0, all patterns 16-step in F2)
    bool barBoundary = false;
    for (int bi = 0; bi < beatEvents.count; ++bi) {
        if (beatEvents.data[bi].beat_index % 16 == 0) { barBoundary = true; break; }
    }

    if (barBoundary) {
        // Apply pending manual switch first
        int pending = pendingIndex_.load(std::memory_order_relaxed);
        if (pending >= 0) {
            current = juce::jlimit(0, numPat - 1, pending);
            playIndex_.store(current, std::memory_order_relaxed);
            pendingIndex_.store(-1, std::memory_order_relaxed);
            chainPosition_ = 0;
            barsRemaining_ = 1;
        } else if (! snap.chain.empty()) {
            // Advance chain
            --barsRemaining_;
            if (barsRemaining_ <= 0) {
                chainPosition_ = (chainPosition_ + 1) % static_cast<int>(snap.chain.size());
                // Find pattern by ID
                const auto& entry = snap.chain[static_cast<std::size_t>(chainPosition_)];
                for (int i = 0; i < numPat; ++i) {
                    if (snap.patterns[static_cast<std::size_t>(i)].id == entry.pattern_id) {
                        current = i; break;
                    }
                }
                barsRemaining_ = juce::jmax(1, snap.chain[static_cast<std::size_t>(chainPosition_)].bars);
                playIndex_.store(current, std::memory_order_relaxed);
            }
        }
    }

    return current;
}
