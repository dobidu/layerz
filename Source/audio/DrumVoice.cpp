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
    phase2_   = 0.0;

    // Retrigger: always reset from start regardless of whether voice was active
    switch (type_) {
        case Type::KICK:
            pitchHz_ = 220.0;
            len_ = static_cast<int>(sampleRate_ * 0.6);  // max 600ms
            break;
        case Type::SNARE: {
            float decayMs = juce::jlimit(30.0f, 200.0f, param1_);
            len_ = static_cast<int>(sampleRate_ * decayMs / 1000.0f);
            break;
        }
        case Type::HAT: {
            float decayMs = juce::jlimit(10.0f, 80.0f, param1_);
            len_ = static_cast<int>(sampleRate_ * decayMs / 1000.0f);
            if (filterPrepared_) {
                // 7kHz highpass Q=1.2 — metallic character
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    sampleRate_, 7000.0f, 1.2f);
                *hatFilter_.coefficients = *coeffs;
                hatFilter_.reset();
            }
            break;
        }
        case Type::PERC:
            len_ = static_cast<int>(sampleRate_ * 0.08);  // 80ms
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
    double pitchLow = 45.0, pitchHigh = 200.0;
    double ampDecaySamples   = sampleRate_ * 0.45;  // 450ms — longer body
    double clickDecaySamples = sampleRate_ * 0.008; // 8ms click punch

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        double pitchNorm = std::exp(-t / pitchDecaySamples);
        pitchHz_ = pitchLow + (pitchHigh - pitchLow) * pitchNorm;
        phase_ += pitchHz_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        float body  = 0.75f * velocity_
                      * static_cast<float>(std::exp(-t / ampDecaySamples))
                      * static_cast<float>(std::sin(kTwoPi * phase_));
        float click = 0.55f * velocity_
                      * static_cast<float>(std::exp(-t / clickDecaySamples))
                      * nextNoise();
        ch[start + i] += body + click;
    }
    pos_ += write;
}

void DrumVoice::processSnare(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    float decayMs = juce::jlimit(30.0f, 200.0f, param1_);
    double decaySamples     = sampleRate_ * decayMs / 1000.0;
    double snapDecaySamples = sampleRate_ * 0.008; // 8ms snap transient

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        float amp  = velocity_ * static_cast<float>(std::exp(-t / decaySamples));
        float snap = velocity_ * static_cast<float>(std::exp(-t / snapDecaySamples));
        // Average 3 LCG samples → warmer, less aliased noise
        float noise = (nextNoise() + nextNoise() + nextNoise()) * 0.333f;
        // Snap transient: brighter single noise burst
        float snapN = nextNoise();
        // Body tone: 200Hz, subtle (10%) — weight without bell-like ring
        phase_ += 200.0 / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        float tone = 0.1f * static_cast<float>(std::sin(kTwoPi * phase_));
        ch[start + i] += 0.6f * amp * noise + 0.4f * snap * snapN + 0.6f * amp * tone;
    }
    pos_ += write;
}

void DrumVoice::processHat(juce::AudioBuffer<float>& buf, int start) noexcept {
    auto* ch = buf.getWritePointer(0);
    float decayMs = juce::jlimit(10.0f, 80.0f, param1_);
    double decaySamples = sampleRate_ * decayMs / 1000.0;

    // Ring modulation of two inharmonic sine waves — metallic cymbal character.
    // f1 × f2 creates sum/difference frequencies with the "clang" of a real hat.
    // f1=8000Hz, f2=10300Hz (inharmonic ratio ≈1.2875) → components at 2300Hz + 18300Hz.
    static constexpr double kHatF1 = 8000.0;
    static constexpr double kHatF2 = 10300.0;

    int avail = buf.getNumSamples() - start;
    int write = juce::jmin(len_ - pos_, avail);

    for (int i = 0; i < write; ++i) {
        double t = static_cast<double>(pos_ + i);
        float amp = velocity_ * static_cast<float>(std::exp(-t / decaySamples));
        phase_  += kHatF1 / sampleRate_;
        phase2_ += kHatF2 / sampleRate_;
        if (phase_  >= 1.0) phase_  -= 1.0;
        if (phase2_ >= 1.0) phase2_ -= 1.0;
        // Ring mod: product of two sines = metallic ring
        float ring = static_cast<float>(std::sin(kTwoPi * phase_)
                                      * std::sin(kTwoPi * phase2_));
        // Mix with a little noise for non-pitched texture
        float noise = nextNoise() * 0.15f;
        ch[start + i] += 0.55f * amp * (ring + noise);
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
