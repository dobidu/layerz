#include "Clock.h"
#include <cmath>

void Clock::prepare(double sampleRate, int maxBlockSize) noexcept {
    sampleRate_   = sampleRate;
    samplesPerBeat_ = sampleRate * 60.0 / bpm_;
    // Pre-allocate: maximum possible beats per block + 1 headroom
    int maxBeatsPerBlock = static_cast<int>(maxBlockSize / samplesPerBeat_) + 2;
    eventBuffer_.resize(static_cast<std::size_t>(maxBeatsPerBlock));
    eventCount_ = 0;
}

void Clock::setBpm(double bpm) noexcept {
    bpm_ = bpm;
    samplesPerBeat_ = sampleRate_ * 60.0 / bpm_;
}

BeatEvents Clock::process(const juce::AudioPlayHead::PositionInfo& pos,
                                          int numSamples) noexcept {
    eventCount_ = 0;

    if (standaloneMode_) {
        // Internal clock — advance sampleCounter_ and emit beats
        if (! playing_) return { eventBuffer_.data(), 0 };

        for (int s = 0; s < numSamples; ++s) {
            if (sampleCounter_ <= 0.0) {
                if (eventCount_ < static_cast<int>(eventBuffer_.size())) {
                    eventBuffer_[static_cast<std::size_t>(eventCount_++)] = { beatIndex_++, s };
                }
                sampleCounter_ += samplesPerBeat_;
            }
            sampleCounter_ -= 1.0;
        }
    } else {
        // Plugin mode — derive beat positions from host PPQ
        if (! pos.getIsPlaying()) return { eventBuffer_.data(), 0 };

        auto bpmOpt = pos.getBpm();
        auto ppqOpt = pos.getPpqPosition();
        if (! bpmOpt.hasValue() || ! ppqOpt.hasValue())
            return { eventBuffer_.data(), 0 };

        double hostBpm      = *bpmOpt;
        double ppqStart     = *ppqOpt;
        double spb          = sampleRate_ * 60.0 / hostBpm;  // samples per beat
        double ppqEnd       = ppqStart + numSamples / spb;

        double beatFloor    = std::floor(ppqStart);
        // Walk through each beat boundary that falls within this block
        for (double beat = (ppqStart <= beatFloor ? beatFloor : beatFloor + 1.0);
             beat < ppqEnd; beat += 1.0) {
            double offsetPpq    = beat - ppqStart;
            int    sampleOffset = static_cast<int>(std::round(offsetPpq * spb));
            sampleOffset = juce::jlimit(0, numSamples - 1, sampleOffset);
            if (eventCount_ < static_cast<int>(eventBuffer_.size())) {
                int idx = static_cast<int>(beat);
                eventBuffer_[static_cast<std::size_t>(eventCount_++)] = { idx, sampleOffset };
            }
        }
    }

    return { eventBuffer_.data(), eventCount_ };
}
