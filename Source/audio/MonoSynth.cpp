#include "MonoSynth.h"
#include <cmath>

void MonoSynth::prepare(const juce::dsp::ProcessSpec& spec) noexcept {
    sampleRate_ = spec.sampleRate;
    phase_      = 0.0;
    active_     = false;
}

void MonoSynth::trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept {
    hz_       = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    velocity_ = velocity;
    waveform_ = params.waveform;
    phase_    = 0.0;
    active_   = true;
}

void MonoSynth::release() noexcept {
    active_ = false;
}

void MonoSynth::process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept {
    if (! active_ || buf.getNumChannels() < 1) return;
    int avail = buf.getNumSamples() - startSample;
    int write = juce::jmin(numSamples, avail);
    if (write <= 0) return;
    auto* ch = buf.getWritePointer(0);
    for (int i = 0; i < write; ++i) {
        phase_ += hz_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        float osc = (waveform_ == Waveform::SAW)
            ? static_cast<float>(phase_ * 2.0 - 1.0)
            : (phase_ < 0.5 ? 1.0f : -1.0f);
        ch[startSample + i] += osc * velocity_ * 0.4f;
    }
}
