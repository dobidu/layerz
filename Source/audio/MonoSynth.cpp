#include "MonoSynth.h"
#include <cmath>

void MonoSynth::prepare(const juce::dsp::ProcessSpec& spec) noexcept {
    sampleRate_ = spec.sampleRate;
    phase_      = 0.0;
    active_     = false;
    releasing_  = false;
    envVal_     = 0.0f;
    envPos_     = 0;
}

void MonoSynth::startEnvelope(float velocity, const BassVoiceParams& params) noexcept {
    velocity_  = velocity;
    waveform_  = params.waveform;
    hz_        = 440.0 * std::pow(2.0, (juce::roundToInt(440.0) == 440 ? 0.0 : 0.0));
    attackSamples_  = juce::jmax(1, (int)(params.env_attack_ms  / 1000.0f * sampleRate_));
    decaySamples_   = juce::jmax(1, (int)(params.env_decay_ms   / 1000.0f * sampleRate_));
    releaseSamples_ = juce::jmax(1, (int)(params.env_release_ms / 1000.0f * sampleRate_));
    sustain_        = juce::jlimit(0.0f, 1.0f, params.env_sustain);
    envPos_         = 0;
    envVal_         = 0.0f;
    releasing_      = false;
    active_         = true;
}

void MonoSynth::trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept {
    hz_    = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    phase_ = 0.0;  // clean phase reset for new notes
    startEnvelope(velocity, params);
}

void MonoSynth::triggerRetain(float velocity, const BassVoiceParams& params) noexcept {
    // Phase NOT reset — preserves waveform continuity (no click at STUTTER sub-events)
    startEnvelope(velocity, params);
}

void MonoSynth::release() noexcept {
    if (! active_) return;
    releasing_  = true;
    releaseFrom_ = envVal_;
    envPos_     = 0;
}

void MonoSynth::process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept {
    if (! active_ || buf.getNumChannels() < 1) return;
    int avail = buf.getNumSamples() - startSample;
    int write = juce::jmin(numSamples, avail);
    if (write <= 0) return;
    auto* ch = buf.getWritePointer(0);

    for (int i = 0; i < write; ++i) {
        // Advance oscillator
        phase_ += hz_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        float osc = (waveform_ == Waveform::SAW)
            ? static_cast<float>(phase_ * 2.0 - 1.0)
            : (phase_ < 0.5 ? 1.0f : -1.0f);

        // Compute ADSR envelope
        if (! releasing_) {
            if      (envPos_ < attackSamples_)
                envVal_ = static_cast<float>(envPos_) / static_cast<float>(attackSamples_);
            else if (envPos_ < attackSamples_ + decaySamples_)
                envVal_ = 1.0f - (static_cast<float>(envPos_ - attackSamples_)
                          / static_cast<float>(decaySamples_)) * (1.0f - sustain_);
            else
                envVal_ = sustain_;
            ++envPos_;
        } else {
            if (envPos_ < releaseSamples_)
                envVal_ = releaseFrom_ * (1.0f - static_cast<float>(envPos_)
                          / static_cast<float>(releaseSamples_));
            else
                envVal_ = 0.0f;
            ++envPos_;
            if (envPos_ >= releaseSamples_) { envVal_ = 0.0f; active_ = false; }
        }

        ch[startSample + i] += osc * envVal_ * velocity_ * 0.4f;
    }
}
