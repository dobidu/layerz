#include "BeatSequencer.h"

void BeatSequencer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
}

void BeatSequencer::process(const Project& snap,
                             const BeatEvents& beatEvents,
                             juce::AudioBuffer<float>& buf,
                             VoiceBank& bank) noexcept {
    if (snap.patterns.empty()) return;
    const auto& pattern = snap.patterns[0];   // 03-02 will replace with patternIndex
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

    // ── BASS layer ───────────────────────────────────────────────────────────
    // Rendering order (audit M3): sustain pre-trigger → trigger+tail → no-trigger full block
    for (const auto& layer : pattern.layers) {
        if (layer.type != LayerType::BASS) continue;

        // Find the earliest BASS beat event in this block
        int firstBassOffset = buf.getNumSamples(); // default = no trigger
        for (int bi = 0; bi < beatEvents.count; ++bi) {
            const auto& beatEv = beatEvents.data[bi];
            int beat_step = beatEv.beat_index % pattern.length_steps;
            bool hasEvent = false;
            for (const auto& event : layer.events)
                if (event.step == beat_step && event.midi_note >= 0) { hasEvent = true; break; }
            if (hasEvent && beatEv.sample_offset < firstBassOffset)
                firstBassOffset = beatEv.sample_offset;
        }

        // 1. Render sustaining note from previous block (0 → firstBassOffset)
        if (firstBassOffset > 0)
            bank.processBass(buf, 0, firstBassOffset);

        // 2. For each BASS trigger: release previous → trigger new → render tail
        if (firstBassOffset < buf.getNumSamples()) {
            for (int bi = 0; bi < beatEvents.count; ++bi) {
                const auto& beatEv = beatEvents.data[bi];
                int beat_step = beatEv.beat_index % pattern.length_steps;
                for (const auto& event : layer.events) {
                    if (event.step == beat_step && event.midi_note >= 0) {
                        bank.releaseBass();
                        bank.triggerBass(event.midi_note, event.velocity, layer.bass_params);
                        bank.processBass(buf, beatEv.sample_offset,
                                         buf.getNumSamples() - beatEv.sample_offset);
                        break;
                    }
                }
            }
        }

        // 3. No BASS trigger this block: render full sustain
        if (firstBassOffset == buf.getNumSamples())
            bank.processBass(buf, 0, buf.getNumSamples());

        break; // only one BASS layer
    }
}
