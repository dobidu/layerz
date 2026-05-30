#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../model/Schema.h"

// Monophonic synthesizer voice for BASS layer.
// Saw/square oscillator → LP filter → ADSR envelope → portamento glide.
// All state allocated in prepare(); zero allocation in trigger/process/release.
// Mono: new trigger() interrupts any currently sounding note (no overlap).
class MonoSynth {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;

    // Audio thread: trigger note. Mono — resets envelope from start.
    // Glide: if glide_ms > 0 AND was previously active, pitch interpolates.
    void trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept;

    // Audio thread: start release phase.
    void release() noexcept;

    // Audio thread: render numSamples into buf ch0 starting at startSample.
    // Clamps to buffer boundary. No allocation.
    void process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;

    bool isActive() const noexcept;

private:
    double sampleRate_ = 44100.0;
    BassVoiceParams params_;
    float velocity_ = 1.0f;
    bool wasActive_ = false;

    double phase_ = 0.0;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> currentPitchHz_;

    juce::ADSR envelope_;
    juce::dsp::StateVariableTPTFilter<float> filter_;
};
