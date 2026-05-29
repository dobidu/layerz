#pragma once
#include <juce_core/juce_core.h>

// RAII guard that marks the audio thread as active.
// Place at the top of processBlock(). Code paths guarded with
// LAYERZ_ASSERT_NOT_AUDIO_THREAD() will fire a jassert if called
// while on the audio thread (e.g. from postMutation, file I/O, operator new).
struct AudioThreadGuard {
    AudioThreadGuard()  noexcept { active() = true;  }
    ~AudioThreadGuard() noexcept { active() = false; }

    static bool& active() noexcept {
        static thread_local bool flag = false;
        return flag;
    }
};

// Assert that the calling code is NOT on the audio thread.
// Use in postMutation, loadFromFile, or any GUI-only path.
#define LAYERZ_ASSERT_NOT_AUDIO_THREAD() \
    jassert(!AudioThreadGuard::active())
