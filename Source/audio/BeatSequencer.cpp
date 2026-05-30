#include "BeatSequencer.h"

void BeatSequencer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 4096;
    spec.numChannels      = 1;
    resolver_.prepare(sampleRate, 4096);
}

void BeatSequencer::process(const Project& snap,
                             int patternIndex,
                             const BeatEvents& beatEvents,
                             double stepPeriodSamples,
                             juce::AudioBuffer<float>& buf,
                             VoiceBank& bank) noexcept {
    if (snap.patterns.empty()) return;
    int pi = juce::jlimit(0, (int)snap.patterns.size()-1, patternIndex);
    const auto& pattern = snap.patterns[static_cast<std::size_t>(pi)];
    if (pattern.layers.empty()) return;

    int blockSize = buf.getNumSamples();

    // Step 1: drain carry-forward events from PREVIOUS block FIRST
    int nPending = resolver_.drainPending(resolvedBuf_, kMaxResolved);
    for (int i = 0; i < nPending; ++i)
        dispatchResolved(resolvedBuf_[i], buf, bank);

    // Step 2: reset FRACTURE seed at pattern loop start (AFTER drain — ordering critical)
    for (int bi = 0; bi < beatEvents.count; ++bi) {
        if (beatEvents.data[bi].beat_index % pattern.length_steps == 0) {
            // Find first BEAT layer seed for reset (use first layer's fracture_seed)
            for (const auto& layer : pattern.layers)
                if (layer.type == LayerType::BEAT && !layer.drum_tracks.empty()) {
                    resolver_.resetFractureSeed(layer.aesthetics.fracture_seed);
                    break;
                }
            break;
        }
    }

    // Step 3: process new beat events with aesthetics
    for (int bi = 0; bi < beatEvents.count; ++bi) {
        const auto& beatEv = beatEvents.data[bi];
        int beat_step = beatEv.beat_index % pattern.length_steps;

        // ── BEAT layer ────────────────────────────────────────────────────────
        for (const auto& layer : pattern.layers) {
            if (layer.type != LayerType::BEAT) continue;
            if (layer.drum_tracks.empty()) continue;

            // Compute effective aesthetics from template + morph (guard empty name)
            Aesthetics neutral{};
            const Aesthetics* tpl = findTemplate(layer.template_name, LayerType::BEAT);
            Aesthetics beatAes = (tpl && layer.morph_amount > 0.0f)
                ? applyMorph(neutral, *tpl, layer.morph_amount)
                : layer.aesthetics;

            for (const auto& track : layer.drum_tracks) {
                if (track.mute) continue;
                for (const auto& event : track.events) {
                    if (event.step != beat_step) continue;
                    float vel = event.velocity * juce::jlimit(0.0f, 1.0f, track.level);
                    int n = resolver_.resolveEvent(
                        false, track.voice_type, event, beatAes,
                        nullptr, vel, track.param1,
                        beatEv.sample_offset, blockSize, stepPeriodSamples,
                        resolvedBuf_, kMaxResolved);
                    for (int k = 0; k < n; ++k)
                        dispatchResolved(resolvedBuf_[k], buf, bank);
                    break;
                }
            }
            break;
        }

        // ── BASS layer ────────────────────────────────────────────────────────
        for (const auto& layer : pattern.layers) {
            if (layer.type != LayerType::BASS) continue;
            // Apply template + morph for BASS
            Aesthetics neutral{};
            const Aesthetics* bTpl = findTemplate(layer.template_name, LayerType::BASS);
            Aesthetics bassAes = (bTpl && layer.morph_amount > 0.0f)
                ? applyMorph(neutral, *bTpl, layer.morph_amount)
                : layer.aesthetics;
            for (const auto& event : layer.events) {
                if (event.step != beat_step || event.midi_note < 0) continue;
                int n = resolver_.resolveEvent(
                    true, "bass", event, bassAes,
                    &layer.bass_params, event.velocity, layer.bass_params.volume,
                    beatEv.sample_offset, blockSize, stepPeriodSamples,
                    resolvedBuf_, kMaxResolved);
                for (int k = 0; k < n; ++k)
                    dispatchResolved(resolvedBuf_[k], buf, bank);
                break;
            }
            break;
        }
    }

    // Step 4: render sustaining BASS across blocks with no new trigger
    bank.processBass(buf, 0, blockSize);
}

void BeatSequencer::dispatchResolved(const ResolvedEvent& ev,
                                      juce::AudioBuffer<float>& buf,
                                      VoiceBank& bank) noexcept {
    int offset = juce::jlimit(0, buf.getNumSamples()-1, ev.sample_offset);
    if (ev.is_bass) {
        if (ev.bass_params) {
            if (ev.velocity < 0.0f) {
                // STUTTER retrigger (negative velocity flag): no phase reset
                bank.retriggerBass(-ev.velocity, *ev.bass_params);
            } else {
                bank.releaseBass();
                bank.triggerBass(ev.midi_note, ev.velocity, *ev.bass_params);
            }
            bank.processBass(buf, offset, buf.getNumSamples() - offset);
        }
    } else {
        bank.trigger(ev.voice_type, ev.velocity, ev.param1, offset, buf);
    }
}
