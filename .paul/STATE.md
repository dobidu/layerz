# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-05-28)

**Core value:** Producers get the immediacy of a groovebox and the aesthetic reach of a full DAW plugin chain — without the friction of assembling one.
**Current focus:** Not yet defined — ready for first plan

## Current Position

Milestone: v1.0 Initial Release (v1.0.0)
Phase: 4 of 8 (F3 — Groove Engine) — Not started
Plan: None yet
Status: Ready to plan
Last activity: 2026-05-30 — Phase 3 (F2) complete, transitioned to Phase 4

Progress:
- Milestone: [███░░░░░░░] 37%
- Phase 3: [██████████] 100% ✅

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [Phase 3 complete — ready for next PLAN]
```

## Accumulated Context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| Thread-safety: C++17 mutex retained | F1 deviation | macOS SDK lacks C++20 atomic<shared_ptr>; revisit when SDK updates |
| Hat synthesis: ring modulation | F1 | f1×f2 inharmonic sines; HP noise removed |
| MonoSynth: minimal oscillator only | F2 deviation | No ADSR/filter yet; stale saved state was root cause of silence; add envelope in F3 |
| saveToFile() was missing impl | F2 bug | Declared in header since F0 but never written; added atomic tmp+rename |
| PROFILE_PLUGIN: 12 voices, 24 grains | F0 | Hardcoded in ProfileConfig.h; recalibrate post-F1 with real Faust |
| PROFILE_STANDALONE: 24 voices, 40 grains | F0 | Same basis |
| UserConfig separate from Project | F0 | LLM key + profile override in platform config dir, never .layerz |
| juce::var/JSON for serialisation | F0 | nlohmann/json deferred to F1 when schema grows |

### Deferred Issues
None yet.

### Blockers/Concerns
None yet.

## Session Continuity

Last session: 2026-05-30
Stopped at: Phase 3 (F2) UNIFY complete — transitioned to Phase 4 (F3)
Next action: /paul:plan for Phase 4 (F3): Groove Engine
Resume context:
  F2 shipped: MonoSynth BASS, ChainManager, pattern selector, PitchedStepRow, save/load.
  MonoSynth is minimal (oscillator only) — add ADSR/filter in F3 alongside groove engine.
  hatFilter_ in DrumVoice is dead code after ring-mod change — remove in F5 cleanup.
  F3 adds: DRAG/PUSH/ROLL/STUTTER/FRACTURE groove aesthetics + template library + morph knob.

---
*STATE.md — Updated after every significant action*
