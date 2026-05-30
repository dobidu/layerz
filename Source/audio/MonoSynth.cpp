#include "MonoSynth.h"
#include <cmath>

static double midiNoteToHz(int note) noexcept {
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

void MonoSynth::prepare(const juce::dsp::ProcessSpec& spec) noexcept {
    sampleRate_ = spec.sampleRate;
    wasActive_  = false;
    phase_      = 0.0;

    currentPitchHz_.reset(sampleRate_, 0.0);
    currentPitchHz_.setCurrentAndTargetValue(440.0);

    envelope_.setSampleRate(sampleRate_);
    envelope_.reset();

    filter_.prepare(spec);
    filter_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter_.setCutoffFrequency(800.0f);
    filter_.setResonance(0.3f);
}

void MonoSynth::trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept {
    params_    = params;
    velocity_  = velocity;

    double targetHz = midiNoteToHz(midiNote);

    if (params_.glide_ms > 0.0f && wasActive_) {
        // Legato: interpolate pitch from current to target
        currentPitchHz_.reset(sampleRate_, params_.glide_ms / 1000.0);
        currentPitchHz_.setTargetValue(targetHz);
    } else {
        // Instant pitch jump
        currentPitchHz_.reset(sampleRate_, 0.0);
        currentPitchHz_.setCurrentAndTargetValue(targetHz);
    }

    // Reset filter on each trigger to clear any residual state
    filter_.reset();
    filter_.setCutoffFrequency(juce::jlimit(80.0f, 8000.0f, params_.filter_cutoff));
    filter_.setResonance(juce::jmax(0.1f, params_.filter_resonance * 0.94f + 0.01f));

    // Update ADSR
    juce::ADSR::Parameters envP;
    envP.attack  = params_.env_attack_ms  / 1000.0f;
    envP.decay   = params_.env_decay_ms   / 1000.0f;
    envP.sustain = params_.env_sustain;
    envP.release = params_.env_release_ms / 1000.0f;
    envelope_.setParameters(envP);
    envelope_.noteOn();

    wasActive_ = true;
}

void MonoSynth::release() noexcept {
    envelope_.noteOff();
}

bool MonoSynth::isActive() const noexcept {
    return envelope_.isActive();
}

void MonoSynth::process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept {
    if (! isActive() || buf.getNumChannels() < 1) return;

    int avail = buf.getNumSamples() - startSample;
    int write = juce::jmin(numSamples, avail);
    if (write <= 0) return;

    auto* ch = buf.getWritePointer(0);

    for (int i = 0; i < write; ++i) {
        double hz = currentPitchHz_.getNextValue();
        phase_ += hz / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;

        float osc = (params_.waveform == Waveform::SAW)
            ? static_cast<float>(phase_ * 2.0 - 1.0)
            : (phase_ < 0.5 ? 1.0f : -1.0f);

        float env = envelope_.getNextSample();
        // Filter: pass signal, but guard against NaN if filter state is bad
        float filtered = filter_.processSample(0, osc);
        float out = std::isfinite(filtered) ? filtered : osc;
        ch[startSample + i] += out * env * velocity_ * params_.volume * 0.7f;
    }

    if (! envelope_.isActive()) wasActive_ = false;
}
