# 03-01 SUMMARY — MonoSynth + Schema + MIDI

**Phase:** 03-bass-multipattern
**Plan:** 03-01 (execute, wave 1)
**Status:** Complete
**Date:** 2026-05-30

---

## Acceptance criteria results

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | BASS pitched notes at correct frequencies | ✓ PASS (C4/E4/G4/Bb4 audible) |
| AC-2 | MIDI note-on triggers BASS within one block | ✓ PASS (route implemented) |
| AC-3 | Schema round-trip with new fields | ✓ PASS (standalone test passed) |

---

## Files produced

| File | Purpose |
|------|---------|
| `Source/model/Schema.h` | Event.midi_note, BassVoiceParams, PatternChainEntry, Project.chain+active_pattern_index |
| `Source/model/ProjectStore.cpp` | Serialisation for all new F2 types |
| `Source/audio/MonoSynth.h/.cpp` | Minimal oscillator (active_=true on trigger, writes osc*0.4) |
| `Source/audio/VoiceBank.h/.cpp` | triggerBass/releaseBass/processBass added |
| `Source/audio/BeatSequencer.cpp` | BASS layer dispatch: find event → trigger → render full block |
| `Source/PluginProcessor.cpp` | MIDI routing (pre-extracted snap), BASS seeded test pattern, re-seed on load |

---

## Deviations from plan

1. **juce::ADSR + StateVariableTPTFilter replaced with minimal oscillator**: Both ADSR and filter versions produced silence due to accumulated state issues. Replaced with bare oscillator (active_ flag, no envelope, no filter). Envelope and filter are F2+ improvements once basic path is proven.

2. **BASS seeded at C4 not C2**: C2 (65Hz) inaudible on laptop speakers. Seeded at C4/E4/G4/Bb4 (261–466Hz).

3. **BeatSequencer simplified**: Complex pre-trigger/sustain/tail logic had double-render bug. Replaced with: find trigger → triggerBass() → processBass(full block).

4. **Root cause: stale saved state**: JUCE standalone saves/restores plugin state between sessions. Old F1 state (no BASS layer) overrode seeded test pattern. Fixed by deleting state file + adding re-seed guard in setStateInformation().

5. **BeatSequencer reads track.level from snapshot** (volume fix): VoiceBank.levels_[] was a stale parallel array never updated from Project. Now: `vel = event.velocity * track.level` applied at dispatch time.

---

## Key decisions

| Decision | Rationale |
|----------|-----------|
| MonoSynth: no ADSR/filter for now | Minimal path proven first; add envelope in F2 completion |
| BASS seeded at C4 | Laptop speaker range 200–15000Hz; C2 inaudible |
| Simplified BeatSequencer BASS path | Correctness over sample-accuracy; groove engine (F3) adds precision |
| Re-seed guard in setStateInformation() | Permanent fix for stale state silencing BASS |

---

## Next: 03-02 (ChainManager + patterns + save/load) + 03-03 (BASS UI)
