#include "DrumVoice.h"
#include <cmath>
#include <random>

static constexpr double kTwoPi = 2.0 * 3.14159265358979323846;

void DrumVoice::setType(const std::string& t) noexcept {
    if      (t == "snare") type_ = Type::SNARE;
    else if (t == "hat")   type_ = Type::HAT;
    else if (t == "perc")  type_ = Type::PERC;
    else                   type_ = Type::KICK;
}

void DrumVoice::prepare(const juce::dsp::ProcessSpec& spec) noexcept {
    sampleRate_ = spec.sampleRate;
    pos_  = 0;
    len_  = 0;
    phase_    = 0.0;
    pitchHz_  = 180.0;
    // Randomize LCG seed so hat/snare don't have identical noise each launch
    std::random_device rd;
    noiseSeed_ = rd.entropy() > 0 ? static_cast<uint32_t>(rd()) : 54321u;

    // Prepare hat highpass filter with coefficients (updated in trigger())
    hatFilter_.prepare(spec);
    filterPrepared_ = true;
}

void DrumVoice::trigger(float velocity, float param1) noexcept {
    velocity_ = velocity;
    param1_   = param1;
    pos_      = 0;
    phase_    = 0.0;

    // Retrigger: always reset from start regardless of whether voice was active
    switch (type_) {
        case Type::KICK:
            pitchHz_ = 180.0;
            len_ = static_cast<int>(sampleRate_ * 0.5);  // max 500ms
            break;
        case Type::SNARE: {
            float decayMs = juce::jlimit(30.0f, 200.0f, param1_);
            len_ = static_cast<int>(sampleRate_ * decayMs / 1000.0f);
            break;
        }
        case Type::HAT: {
            float decayMs = juce::jlimit(10.0f, 80.0f, param1_);
            len_ = static_cast<int>(sampleRate_ * decayMs / 1000.0f);
            // Update hat filter coefficients for this trigger
            if (filterPrepared_) {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    sampleRate_, 8000.0f, 0.7f);
                *hatFilter_.coefficients = *coeffs;
                hatFilter_.reset();
            }
            break;
        }
        case Type::PERC:
            len_ = static_cast<int>(sampleRate_ * 0.05);  // 50ms fixed
            break;
    }
}

void DrumVoice::process(juce::AudioBuffer<float>& buf, int startSample) noexcept {
    if (pos_ >= len_ || buf.getNumChannels() < 1) return;
    switch (type_) {
        case Type::KICK:  processKick (buf, startSample); break;
        case Type::SNARE: processSnare(buf, startSample); break;
        case Type::HAT:   processHat  (buf, startSample); break;
        case Type::PERC:  processPerc (buf, startSample); break;
    }
}

void DrumVoice::processKick(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    float decayMs = juce::jlimit(20.0f, 500.0f, param1_);
    double pitchDecaySamples = sampleRate_ * decayMs / 1000.0;
    double pitchLow = 50.0, pitchHigh = 180.0;
    double ampDecaySamples = sampleRate_ * 0.4;  // 400ms amplitude decay

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        // Exponential pitch sweep
        double pitchNorm = std::exp(-t / pitchDecaySamples);
        pitchHz_ = pitchLow + (pitchHigh - pitchLow) * pitchNorm;
        // Advance phase
        phase_ += pitchHz_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        // Amplitude envelope
        float amp = velocity_ * static_cast<float>(std::exp(-t / ampDecaySamples));
        ch[start + i] += amp * static_cast<float>(std::sin(kTwoPi * phase_));
    }
    pos_ += write;
}

void DrumVoice::processSnare(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    float decayMs = juce::jlimit(30.0f, 200.0f, param1_);
    double decaySamples = sampleRate_ * decayMs / 1000.0;

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        float amp = velocity_ * static_cast<float>(std::exp(-t / decaySamples));
        // Noise component (LCG — deterministic, zero stdlib overhead)
        float noise = nextNoise();
        // Tone component: ~220Hz sine at 30% level
        phase_ += 220.0 / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        float tone = 0.3f * static_cast<float>(std::sin(kTwoPi * phase_));
        ch[start + i] += amp * (noise * 0.7f + tone);
    }
    pos_ += write;
}

void DrumVoice::processHat(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    float decayMs = juce::jlimit(10.0f, 80.0f, param1_);
    double decaySamples = sampleRate_ * decayMs / 1000.0;

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        float amp = velocity_ * static_cast<float>(std::exp(-t / decaySamples));
        float noise = nextNoise();
        float filtered = filterPrepared_ ? hatFilter_.processSample(noise) : noise;
        ch[start + i] += amp * filtered;
    }
    pos_ += write;
}

void DrumVoice::processPerc(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    double decaySamples = sampleRate_ * 0.05;  // 50ms
    float  freq = juce::jlimit(60.0f, 800.0f, param1_);

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        float amp = velocity_ * static_cast<float>(std::exp(-t / decaySamples));
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        ch[start + i] += amp * static_cast<float>(std::sin(kTwoPi * phase_));
    }
    pos_ += write;
}
