#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../model/Schema.h"

// Minimal monophonic oscillator for BASS layer.
// No ADSR, no filter — just a running oscillator that sounds when active.
class MonoSynth {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;
    void trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept;
    void release() noexcept;
    void process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;
    bool isActive() const noexcept { return active_; }

private:
    double sampleRate_ = 44100.0;
    double phase_      = 0.0;
    double hz_         = 440.0;
    float  velocity_   = 1.0f;
    bool   active_     = false;
    Waveform waveform_ = Waveform::SAW;
};
