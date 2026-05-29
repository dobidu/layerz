#pragma once
#include "../model/Schema.h"

// Caps derived from F0.99 spike (docs/spikes/F0.99-performance-budget.md).
// Weakest measured machine: Lenovo i7-8565U.
// Basis: 10x real-DSP-cost multiplier over synthetic sine benchmark.
//
// PRELIMINARY — recalibrate after F1 when real Faust drum voice exists.
// Methodology: replace VoiceBench synthetic voices with Faust shim, re-sweep.

struct ProfileConfig {
    int max_voices;
    int max_grains;  // BLOOM + PULVERIZE combined

    static ProfileConfig forPlugin() noexcept {
        // Lenovo buf=256: 12v+24g = 2.04% bench → ~20% real at 10x
        return { 12, 24 };
    }

    static ProfileConfig forStandalone() noexcept {
        // Lenovo buf=1024: 24v+40g = 3.63% bench → ~36% real at 10x
        return { 24, 40 };
    }

    static ProfileConfig forProfile(Profile p) noexcept {
        if (p == Profile::STANDALONE) return forStandalone();
        return forPlugin();  // AUTO defaults to plugin-safe caps
    }
};
