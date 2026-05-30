#include "VoiceBank.h"

static const char* kVoiceNames[4] = { "kick", "snare", "hat", "perc" };

int VoiceBank::typeIndex(const std::string& t) noexcept {
    for (int i = 0; i < 4; ++i)
        if (t == kVoiceNames[i]) return i;
    return -1;
}

void VoiceBank::prepare(double sampleRate, int maxBlockSize) noexcept {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<uint32_t>(maxBlockSize);
    spec.numChannels      = 1;

    for (int i = 0; i < kVoiceCount; ++i) {
        voices_[i].setType(kVoiceNames[i]);
        voices_[i].prepare(spec);
    }
    bass_.prepare(spec);
}

void VoiceBank::triggerBass(int midiNote, float velocity, const BassVoiceParams& p) noexcept {
    bass_.trigger(midiNote, velocity, p);
}
void VoiceBank::releaseBass() noexcept { bass_.release(); }
void VoiceBank::processBass(juce::AudioBuffer<float>& buf, int startSample, int n) noexcept {
    bass_.process(buf, startSample, n);
}

void VoiceBank::trigger(const std::string& voiceType, float velocity, float param1,
                        int sampleOffset, juce::AudioBuffer<float>& buf) noexcept {
    int idx = typeIndex(voiceType);
    if (idx < 0) return;
    if (mutes_[idx]) return;                // mute check before trigger

    voices_[idx].trigger(velocity * levels_[idx], param1);
    voices_[idx].process(buf, sampleOffset);
}

void VoiceBank::setLevel(const std::string& type, float level) noexcept {
    int idx = typeIndex(type);
    if (idx >= 0) levels_[idx] = juce::jlimit(0.0f, 1.0f, level);
}

void VoiceBank::setMute(const std::string& type, bool muted) noexcept {
    int idx = typeIndex(type);
    if (idx >= 0) mutes_[idx] = muted;
}
