#include "BeatSequencer.h"

void BeatSequencer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
}

void BeatSequencer::process(const Project& snap,
                             int patternIndex,
                             const BeatEvents& beatEvents,
                             juce::AudioBuffer<float>& buf,
                             VoiceBank& bank) noexcept {
    if (snap.patterns.empty()) return;
    int pi = juce::jlimit(0, (int)snap.patterns.size()-1, patternIndex);
    const auto& pattern = snap.patterns[static_cast<std::size_t>(pi)];
    if (pattern.layers.empty()) return;

    // ── BEAT layer ──────────────────────────────────────────────────────────
    for (const auto& layer : pattern.layers) {
        if (layer.type != LayerType::BEAT) continue;
        if (layer.drum_tracks.empty()) continue;

        for (int bi = 0; bi < beatEvents.count; ++bi) {
            const auto& beatEv = beatEvents.data[bi];
            int beat_step = beatEv.beat_index % pattern.length_steps;

            for (const auto& track : layer.drum_tracks) {
                if (track.mute) continue;
                for (const auto& event : track.events) {
                    if (event.step == beat_step) {
                        // Apply level from snapshot (not VoiceBank's stale array)
                        float vel = event.velocity * juce::jlimit(0.0f, 1.0f, track.level);
                        bank.trigger(track.voice_type, vel, track.param1,
                                     beatEv.sample_offset, buf);
                        break;
                    }
                }
            }
        }
        break; // only one BEAT layer
    }

    // ── BASS layer (simple: trigger at block start, render full block) ──────
    for (const auto& layer : pattern.layers) {
        if (layer.type != LayerType::BASS) continue;

        // Check if any BASS event fires this block; trigger if so
        for (int bi = 0; bi < beatEvents.count; ++bi) {
            const auto& beatEv = beatEvents.data[bi];
            int beat_step = beatEv.beat_index % pattern.length_steps;
            for (const auto& event : layer.events) {
                if (event.step == beat_step && event.midi_note >= 0) {
                    bank.triggerBass(event.midi_note, event.velocity, layer.bass_params);
                    break;
                }
            }
        }

        // Always render full block (sustain or new note)
        bank.processBass(buf, 0, buf.getNumSamples());
        break;
    }
}
