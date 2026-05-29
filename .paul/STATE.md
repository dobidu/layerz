# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-05-28)

**Core value:** Producers get the immediacy of a groovebox and the aesthetic reach of a full DAW plugin chain — without the friction of assembling one.
**Current focus:** Not yet defined — ready for first plan

## Current Position

Milestone: v1.0 Initial Release (v1.0.0)
Phase: 2 of 8 (F1 — BEAT Layer) — Not started
Plan: None yet
Status: Ready to plan
Last activity: 2026-05-29 — Phase 1 (F0) complete, transitioned to Phase 2

Progress:
- Milestone: [█░░░░░░░░░] 12%
- Phase 1: [██████████] 100% ✅

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [Phase 1 complete — ready for next PLAN]
```

## Accumulated Context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| Thread-safety: lock-free-snapshot (C++17 short-mutex) | F0 | Audio thread reads via snapshot(); upgrade to C++20 atomic in F1 |
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
Stopped at: Phase 1 (F0) UNIFY complete — transitioned to Phase 2 (F1)
Next action: /paul:plan — Phase 2 (F1): BEAT Layer
Resume context:
  Build green on macOS. CI (Linux/Windows) push pending verification.
  ProjectStore, Clock, Schema v1, ProfileConfig, UserConfig all in place.
  F1 can read patterns via store_.snapshot() and dispatch to beat clock.
  ProfileConfig caps are PRELIMINARY — recalibrate after F1 Faust drum voice exists.

---
*STATE.md — Updated after every significant action*
