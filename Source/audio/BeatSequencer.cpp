#include "BeatSequencer.h"

void BeatSequencer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
}

void BeatSequencer::process(const Project& snap,
                             const BeatEvents& beatEvents,
                             juce::AudioBuffer<float>& buf,
                             VoiceBank& bank) noexcept {
    // Full bounds guard chain (audit M4)
    if (snap.patterns.empty()) return;
    const auto& pattern = snap.patterns[0];
    if (pattern.layers.empty()) return;
    const auto& layer = pattern.layers[0];
    if (layer.type != LayerType::BEAT) return;
    if (layer.drum_tracks.empty()) return;

    for (int bi = 0; bi < beatEvents.count; ++bi) {
        const auto& beatEv = beatEvents.data[bi];
        int beat_step = beatEv.beat_index % pattern.length_steps;

        for (const auto& track : layer.drum_tracks) {
            if (track.mute) continue;
            // Linear scan for first matching step (audit M5: break after first match)
            for (const auto& event : track.events) {
                if (event.step == beat_step) {
                    bank.trigger(track.voice_type, event.velocity, track.param1,
                                 beatEv.sample_offset, buf);
                    break; // do NOT double-trigger on duplicate events
                }
            }
        }
    }
}
