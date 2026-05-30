# Roadmap: LAYERZ

## Overview

LAYERZ v1 ships as a standalone groovebox + VST3/AU/LV2 plugin across Linux/macOS/Windows in eight phases: scaffolding (F0), one layer at a time (F1–F4), cross-cutting FX (F5), LLM bridge (F6), and distribution (F7). Each phase is a vertical slice — partial completion still yields a usable instrument.

## Current Milestone

**v1.0 Initial Release** (v1.0.0)
Status: In progress
Phases: 2 of 8 complete

## Phases

| Phase | Name | Plans | Status | Completed |
|-------|------|-------|--------|-----------|
| 1 (F0) | Scaffolding + Spikes | 3 | ✅ Complete | 2026-05-29 |
| 2 (F1) | BEAT Layer | 2 | ✅ Complete | 2026-05-29 |
| 3 (F2) | BASS Layer + Multi-pattern | TBD | Not started | - |
| 4 (F3) | Groove Engine | TBD | Not started | - |
| 5 (F4) | HARMONIC + MELODIC | TBD | Not started | - |
| 6 (F5) | Cross-cutting FX & Spatial | TBD | Not started | - |
| 7 (F6) | LLM Bridge | TBD | Not started | - |
| 8 (F7) | Distribution | TBD | Not started | - |

## Phase Details

### Phase 1 (F0): Scaffolding + Spikes

**Goal:** Plugin and standalone build cleanly on all three OSes; audio thread produces stable click; Project model exists with thread-safe access pattern decided.
**Depends on:** Nothing (first phase)
**Research:** Likely — two spikes required

**Spikes:**
- F0.99: Performance budget (voice + grain caps at 256/512/1024-sample buffers, across 3 machines)
- F0.99b: Thread-safety model (lock-free snapshot vs message-passed vs double-buffered)

**Scope:**
- JUCE + CMake project setup, all plugin formats + standalone
- CI for Linux/macOS/Windows
- Stub Project model with JSON I/O
- Audio thread harness with metronome click
- Host sync (PPQ → internal step counter)
- Performance benchmark harness
- Profile switching (PROFILE_PLUGIN / PROFILE_STANDALONE)

**Plans:**
- [x] 01-01: Spikes — F0.99 (perf budget: 12v/24g plugin, 24v/40g standalone) + F0.99b (lock-free-snapshot)
- [x] 01-02: CMake/JUCE 8.0.4 + GitHub Actions CI — VST3/AU/LV2/Standalone builds
- [x] 01-03: ProjectStore, Schema.h v1, Clock, ProfileConfig, UserConfig, PluginProcessor wired

### Phase 2 (F1): BEAT Layer

**Goal:** Single BEAT layer plays 16-step pattern with 3–4 drum voices at locked-grid timing. GUI shows layer strip. Instrument makes a usable beat.
**Depends on:** Phase 1 (F0 — build pipeline, audio thread, Project model)
**Research:** Unlikely (F0 spike results inform voice caps)

**Scope:**
- Drum voice synthesis in Faust (kick, snare, hat, perc)
- Step sequencer reading from Layer.events[]
- Step row UI Component
- Voice parameter panel (slide-out)
- Per-layer level + mute/solo

**Plans:**
- [x] 02-01: DrumVoice (kick/snare/hat ring-mod/perc), VoiceBank, BeatSequencer, AudioThreadGuard
- [x] 02-02: StepRow, VoiceParamPanel, BeatLayerStrip, Editor + transport sync, step indicator

### Phase 3 (F2): BASS Layer + Multi-pattern + Chain

**Goal:** BASS layer (mono-synth). Multiple patterns per project. Basic chain (A→B→A). Save/load .layerz file.
**Depends on:** Phase 2 (F1 — BEAT layer, step sequencer)
**Research:** Unlikely

**Plans:**
- [ ] 03-01: TBD during /paul:plan

### Phase 4 (F3): Groove Engine

**Goal:** DRAG, PUSH, ROLL, STUTTER, FRACTURE functional on BEAT+BASS. Template library (5–8 per layer). Morph knob. 30-second validation recording achievable.
**Depends on:** Phase 3 (F2 — multi-layer, save/load)
**Research:** Likely — F3.99 spike (groove accuracy under host clock)

**Plans:**
- [ ] 04-01: TBD during /paul:plan

### Phase 5 (F4): HARMONIC + MELODIC

**Goal:** Additive + BLOOM granular voice (HARMONIC), mono/poly scale-constrained voice (MELODIC). Full four-layer music possible. 2-minute piece achievable.
**Depends on:** Phase 4 (F3 — groove engine)
**Research:** Likely — F4.99 spike (additive voice cost)

**Plans:**
- [ ] 05-01: TBD during /paul:plan

### Phase 6 (F5): Cross-cutting FX & Spatial

**Goal:** PULVERIZE, HARDPAN, PINGPONG, AUTOPAN, FLUX all integrated per-layer and master. Aesthetic range from "broken" to "destroyed" to "organic".
**Depends on:** Phase 5 (F4 — all four layers)
**Research:** Likely — F5.99 spike (PULVERIZE algorithm)

**Plans:**
- [ ] 06-01: TBD during /paul:plan

### Phase 7 (F6): LLM Bridge

**Goal:** Natural-language prompts → validated PatternPatches → user previews and applies. Provider-neutral, tested against ≥1 major API.
**Depends on:** Phase 6 (F5 — complete Project model, FX chain)
**Research:** Likely — F6.99 spike (PatternPatch schema + LLM prompt design)

**Plans:**
- [ ] 07-01: TBD during /paul:plan

### Phase 8 (F7): Distribution

**Goal:** LAYERZ v1 downloadable, installable, documented. Beta with 3–5 producers complete.
**Depends on:** Phase 7 (F6 — LLM bridge complete)
**Research:** Unlikely

**Plans:**
- [ ] 08-01: TBD during /paul:plan

---
*Roadmap created: 2026-05-28*
*Full roadmap detail: ./ROADMAP.md (root)*
