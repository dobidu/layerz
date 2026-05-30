#include "MonoSynth.h"
#include <cmath>

static double midiNoteToHz(int note) noexcept {
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

void MonoSynth::prepare(const juce::dsp::ProcessSpec& spec) noexcept {
    sampleRate_ = spec.sampleRate;
    phase_      = 0.0;
    envPos_     = 0;
    envLen_     = 0;
    releasing_  = false;
    envVal_     = 0.0f;
    targetHz_   = 440.0;
    currentHz_  = 440.0;
}

void MonoSynth::trigger(int midiNote, float velocity, const BassVoiceParams& p) noexcept {
    params_    = p;
    velocity_  = velocity;
    targetHz_  = midiNoteToHz(midiNote);

    // Instant pitch (glide in F3+)
    currentHz_ = targetHz_;

    // Simple manual ADSR — no juce::ADSR dependency
    attackSamples_  = static_cast<int>(p.env_attack_ms  / 1000.0f * static_cast<float>(sampleRate_));
    decaySamples_   = static_cast<int>(p.env_decay_ms   / 1000.0f * static_cast<float>(sampleRate_));
    releaseSamples_ = static_cast<int>(p.env_release_ms / 1000.0f * static_cast<float>(sampleRate_));
    sustain_        = juce::jlimit(0.0f, 1.0f, p.env_sustain);

    if (attackSamples_ < 1)  attackSamples_  = 1;
    if (decaySamples_ < 1)   decaySamples_   = 1;
    if (releaseSamples_ < 1) releaseSamples_ = 1;

    envPos_    = 0;
    releasing_ = false;
    envVal_    = 0.0f;
}

void MonoSynth::release() noexcept {
    if (isActive()) {
        releasing_  = true;
        releaseFrom_ = envVal_;
        envPos_     = 0;
    }
}

bool MonoSynth::isActive() const noexcept {
    return envLen_ > 0 || envPos_ > 0 || envVal_ > 0.0001f;
}

void MonoSynth::process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept {
    if (buf.getNumChannels() < 1) return;

    int avail = buf.getNumSamples() - startSample;
    int write = juce::jmin(numSamples, avail);
    if (write <= 0) return;

    // If note was just triggered (envPos_=0, not releasing), we ARE active
    bool justTriggered = (!releasing_ && envVal_ < 0.0001f && envPos_ == 0
                          && attackSamples_ > 0);
    if (! justTriggered && envVal_ < 0.0001f && releasing_) return; // fully silent

    auto* ch = buf.getWritePointer(0);

    for (int i = 0; i < write; ++i) {
        // Advance oscillator
        phase_ += currentHz_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;

        float osc = (params_.waveform == Waveform::SAW)
            ? static_cast<float>(phase_ * 2.0 - 1.0)
            : (phase_ < 0.5 ? 1.0f : -1.0f);

        // Manual ADSR envelope
        if (! releasing_) {
            if (envPos_ < attackSamples_) {
                envVal_ = static_cast<float>(envPos_) / static_cast<float>(attackSamples_);
            } else if (envPos_ < attackSamples_ + decaySamples_) {
                float t = static_cast<float>(envPos_ - attackSamples_)
                          / static_cast<float>(decaySamples_);
                envVal_ = 1.0f - t * (1.0f - sustain_);
            } else {
                envVal_ = sustain_;
            }
            ++envPos_;
        } else {
            if (envPos_ < releaseSamples_) {
                float t = static_cast<float>(envPos_) / static_cast<float>(releaseSamples_);
                envVal_ = releaseFrom_ * (1.0f - t);
            } else {
                envVal_ = 0.0f;
            }
            ++envPos_;
        }

        ch[startSample + i] += osc * envVal_ * velocity_ * params_.volume * 0.7f;
    }

    // Mark as inactive if release finished
    if (releasing_ && envPos_ >= releaseSamples_) {
        envVal_ = 0.0f;
        envPos_ = 0;
        envLen_ = 0;
    }
}
