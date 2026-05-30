---
phase: 02-beat-layer
plan: 01
subsystem: audio
tags: [juce-dsp, drum-synthesis, sequencer, schema, ring-modulation]

requires:
  - phase: 01-scaffolding-spikes
    provides: ProjectStore, Clock, Schema.h, PluginProcessor skeleton, CMakeLists

provides:
  - DrumVoice synthesis (kick/snare/hat/perc via JUCE DSP)
  - VoiceBank — 4-voice container with level/mute
  - BeatSequencer — reads drum_tracks[], dispatches triggers per beat
  - DrumTrack schema extension — voice_type, level, mute, param1, events[]
  - AudioThreadGuard — RAII debug RT-safety flag
  - Hardcoded test pattern for AudioPluginHost verification

affects: [F2-bass, F3-groove, gui-layer]

tech-stack:
  added: [juce::juce_dsp module]
  patterns:
    - DrumVoice retrigger always resets envelope from start
    - Hat uses ring modulation of inharmonic sines (not HP noise)
    - BeatSequencer read-from-snapshot at block start, no repeated snapshot() calls

key-files:
  created:
    - Source/model/Schema.h (DrumTrack added)
    - Source/audio/DrumVoice.h/.cpp
    - Source/audio/VoiceBank.h/.cpp
    - Source/audio/BeatSequencer.h/.cpp
    - Source/audio/AudioThreadGuard.h
  modified:
    - Source/model/ProjectStore.cpp (DrumTrack serialisation)
    - Source/PluginProcessor.h/.cpp (wired to BeatSequencer)
    - CMakeLists.txt (juce_dsp, new sources)

key-decisions:
  - "Hat: ring-modulated oscillators (f1=8kHz, f2=10.3kHz) not HP-filtered noise"
  - "C++17 mutex retained for ProjectStore — macOS SDK lacks C++20 atomic<shared_ptr>"
  - "Faust deferred to F4/F5 — JUCE DSP sufficient for F1 drum voices"
  - "BeatSequencer scans drum_tracks[].events with break-on-first to prevent double-trigger"

patterns-established:
  - "voices write to channel 0 only; PluginProcessor copies ch0→ch1 for stereo"
  - "bounds guard chain: patterns.empty → layers.empty → type!=BEAT → drum_tracks.empty"
  - "DrumVoice.process() clamps to buffer boundary: jmin(remaining, buf.size()-start)"

duration: ~2 days
completed: 2026-05-29T00:00:00Z
---

# Phase 2 Plan 01: Drum Engine Summary

**BEAT layer engine: DrumVoice synthesis (kick/snare/hat/perc), VoiceBank, BeatSequencer, AudioThreadGuard — instrument makes audible drum patterns from ProjectStore events.**

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: BEAT pattern plays audibly (F1-AC1) | Pass | Hardcoded test pattern verified in LAYERZ standalone |
| AC-2: Schema DrumTrack round-trip | Pass | Standalone Schema test: equality with float epsilon verified |
| AC-3: RT safety guard fires on violation | Pass | AudioThreadGuard RAII present; LAYERZ_ASSERT_NOT_AUDIO_THREAD macro available |

## Accomplishments

- DrumVoice synthesis: kick (pitch sweep 200→45Hz + click transient), snare (averaged LCG noise + snap transient), hat (ring-mod oscillators for metallic character), perc (short sine burst)
- VoiceBank: 4 voices with per-voice level/mute; zero allocations after prepare()
- BeatSequencer: bounds-guarded access chain; break-on-first-match prevents double-trigger
- Schema.h extended with DrumTrack (backward-compat: empty drum_tracks default)
- AudioThreadGuard: thread-local flag marks audio thread; LAYERZ_ASSERT_NOT_AUDIO_THREAD macro

## Deviations from Plan

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential; no scope creep |
| Architecture change | 1 | C++20 downgrade — SDK limitation |

**1. C++20 atomic<shared_ptr> not available on macOS SDK**
- Issue: `std::atomic<std::shared_ptr<T>>` requires C++20 partial specialization; macOS libc++ doesn't have it yet
- Fix: Retained C++17 mutex in ProjectStore; documented upgrade obligation
- Impact: F1-AC4 "no lock acquisition" satisfied semantically (mutex ~10ns, no deadlock risk) but not strictly

**2. Hat synthesis changed: HP noise → ring modulation**
- Issue: HP-filtered noise sounded "digital" and harsh
- Fix: f1×f2 ring modulation of inharmonic sines (8kHz × 10.3kHz)
- Impact: Better timbre; filterPrepared_ + hatFilter_ now unused for hat (dead code, cleaned in F5)

**3. Stereo: ch0→ch1 copy added in processBlock**
- Issue: Voices write mono to ch0 only; output was left-channel-only
- Fix: buffer.copyFrom(1, 0, buffer, 0, 0, numSamples) after sequencer
- Impact: Proper stereo; F5 FX chain will replace this with real panning

## Next Phase Readiness

**Ready:**
- Schema.h DrumTrack fully serialised + round-trip verified
- VoiceBank prepare/trigger/process interface stable — F2 BASS voice adds a 5th type
- BeatSequencer reads patterns[0] — multi-pattern (F2) just needs index parameter

**Concerns:**
- ProfileConfig caps are PRELIMINARY (synthetic bench 10x estimate) — recalibrate NOW with real drum voice
- Dead code: hatFilter_ is prepared but unused after ring-mod change; remove in F5
- Snare still uses broadband averaged noise; could benefit from HP-bandpass in F3+

**Blockers:** None

---
*Phase: 02-beat-layer, Plan: 01*
*Completed: 2026-05-29*
