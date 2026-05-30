#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../model/Schema.h"

// Monophonic synthesizer for BASS layer.
// Saw/square oscillator + manual ADSR envelope.
// trigger() resets phase (clean new note).
// triggerRetain() resets envelope but preserves phase — used by STUTTER to avoid click.
class MonoSynth {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) noexcept;
    void trigger(int midiNote, float velocity, const BassVoiceParams& params) noexcept;
    void triggerRetain(float velocity, const BassVoiceParams& params) noexcept;
    void release() noexcept;
    void process(juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;
    bool isActive() const noexcept { return active_; }

private:
    void startEnvelope(float velocity, const BassVoiceParams& params) noexcept;

    double  sampleRate_ = 44100.0;
    double  phase_      = 0.0;
    double  hz_         = 440.0;
    float   velocity_   = 1.0f;
    bool    active_     = false;
    Waveform waveform_  = Waveform::SAW;

    // Manual ADSR
    int   attackSamples_  = 1;
    int   decaySamples_   = 1;
    int   releaseSamples_ = 1;
    float sustain_        = 0.8f;
    int   envPos_         = 0;
    float envVal_         = 0.0f;
    float releaseFrom_    = 0.0f;
    bool  releasing_      = false;
};
