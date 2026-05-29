#pragma once
#include <juce_dsp/juce_dsp.h>
#include <string>
#include <cstdint>

// Single drum voice — one instance per drum type (kick/snare/hat/perc).
// All synthesis uses JUCE DSP primitives. Zero allocations after prepare().
// trigger() resets the voice envelope from the start on every call (retrigger).
class DrumVoice {
public:
    void setType(const std::string& type) noexcept;
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;

    // Audio thread: resets envelope and synthesis state. No allocation, no lock.
    // Retrigger behavior: always resets from start — correct for drum machines.
    void trigger(float velocity, float param1) noexcept;

    // Audio thread: adds rendered output into buf channel 0 starting at startSample.
    // Clamps to buffer boundary: writes min(remainingSamples, buf.getNumSamples()-startSample).
    void process(juce::AudioBuffer<float>& buf, int startSample) noexcept;

    bool isActive() const noexcept { return pos_ < len_; }

private:
    enum class Type { KICK, SNARE, HAT, PERC } type_ = Type::KICK;

    double sampleRate_  = 44100.0;
    float  velocity_    = 1.0f;
    float  param1_      = 100.0f;

    // Envelope state
    int   pos_ = 0;    // current sample position within voice
    int   len_ = 0;    // total envelope length in samples

    // Synthesis state — pre-allocated, never reallocated after prepare()
    double phase_   = 0.0;   // primary oscillator phase [0..1]
    double phase2_  = 0.0;   // second oscillator (hat ring modulation, snare body)
    double pitchHz_ = 180.0; // current pitch (mutated by kick envelope)

    // Hat filter (pre-allocated coefficients, updated in trigger())
    juce::dsp::IIR::Filter<float> hatFilter_;
    bool filterPrepared_ = false;

    // LCG noise state — seeded once in prepare()
    uint32_t noiseSeed_ = 12345u;
    float    nextNoise() noexcept {
        noiseSeed_ = noiseSeed_ * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(noiseSeed_)) / 2147483648.0f;
    }

    void processKick  (juce::AudioBuffer<float>& buf, int start) noexcept;
    void processSnare (juce::AudioBuffer<float>& buf, int start) noexcept;
    void processHat   (juce::AudioBuffer<float>& buf, int start) noexcept;
    void processPerc  (juce::AudioBuffer<float>& buf, int start) noexcept;
};
