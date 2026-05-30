#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../model/Schema.h"

// Monophonic synthesizer for BASS layer.
// Saw/square oscillator with manual ADSR (no juce::ADSR dependency).
// Mono: new trigger() resets from start (retrigger).
class MonoSynth {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;
    void trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept;
    void release() noexcept;
    void process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;
    bool isActive() const noexcept;

private:
    double      sampleRate_ = 44100.0;
    BassVoiceParams params_;
    float       velocity_    = 1.0f;
    double      phase_       = 0.0;
    double      targetHz_    = 440.0;
    double      currentHz_   = 440.0;

    // Manual ADSR state
    int   attackSamples_  = 0;
    int   decaySamples_   = 0;
    int   releaseSamples_ = 0;
    float sustain_        = 0.8f;
    int   envPos_         = 0;
    int   envLen_         = 0;    // unused — kept for isActive compat
    float envVal_         = 0.0f;
    float releaseFrom_    = 0.0f;
    bool  releasing_      = false;
};
