# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-05-28)

**Core value:** Producers get the immediacy of a groovebox and the aesthetic reach of a full DAW plugin chain — without the friction of assembling one.
**Current focus:** Not yet defined — ready for first plan

## Current Position

Milestone: v1.0 Initial Release (v1.0.0)
Phase: 3 of 8 (F2 — BASS Layer + Multi-pattern) — Not started
Plan: None yet
Status: Ready to plan
Last activity: 2026-05-29 — Phase 2 (F1) complete, transitioned to Phase 3

Progress:
- Milestone: [██░░░░░░░░] 25%
- Phase 2: [██████████] 100% ✅

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [Phase 2 complete — ready for next PLAN]
```

## Accumulated Context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| Thread-safety: C++17 mutex retained | F1 deviation | macOS SDK lacks C++20 atomic<shared_ptr>; revisit when SDK updates |
| Hat synthesis: ring modulation | F1 | f1×f2 inharmonic sines; HP noise removed; dead code in DrumVoice (hatFilter_) |
| PROFILE_PLUGIN: 12 voices, 24 grains | F0 | Hardcoded in ProfileConfig.h; recalibrate post-F1 with real Faust |
| PROFILE_STANDALONE: 24 voices, 40 grains | F0 | Same basis |
| UserConfig separate from Project | F0 | LLM key + profile override in platform config dir, never .layerz |
| juce::var/JSON for serialisation | F0 | nlohmann/json deferred to F1 when schema grows |

### Deferred Issues
None yet.

### Blockers/Concerns
None yet.

## Session Continuity

Last session: 2026-05-29
Stopped at: Phase 2 (F1) UNIFY complete — transitioned to Phase 3 (F2)
Next action: /paul:plan for Phase 3 (F2): BASS Layer + Multi-pattern + Chain
Resume context:
  F1 shipped: drum engine + BEAT UI + step indicator + stereo + transport sync.
  ProfileConfig caps STILL PRELIMINARY — recalibrate with real drum voice benchmark (F1 obligation).
  Dead code: hatFilter_ in DrumVoice unused after ring-mod change — remove in F5 cleanup.
  F2 adds BASS mono-synth voice, multiple patterns, pattern chain, save/load .layerz.

---
*STATE.md — Updated after every significant action*
