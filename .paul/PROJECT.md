# LAYERZ

## What This Is

LAYERZ is a layer-oriented groovebox built as a JUCE/C++ desktop application that ships simultaneously as a standalone instrument and as VST3/AU/LV2 plugin. Music is constructed bottom-up in four stacked layers — BEAT, BASS, HARMONIC, MELODIC — each with a deliberately narrow but deep set of sound generators and a focused aesthetic toolkit. Off-grid groove aesthetics (DRAG/PUSH/ROLL/STUTTER/FRACTURE), granular voice (BLOOM), and destructive FX (PULVERIZE, HARDPAN, PINGPONG, AUTOPAN, FLUX) are first-class citizens. An LLM bridge enables natural-language pattern generation via validated PatternPatches.

## Core Value

Producers get the immediacy of a groovebox and the aesthetic reach of a full DAW plugin chain — without the friction of assembling one.

## Current State

| Attribute | Value |
|-----------|-------|
| Type | Application |
| Version | 0.0.0 |
| Status | Pre-scaffolding |
| Last Updated | 2026-05-28 |

## Requirements

### Core Features

- Four-layer step sequencer (BEAT, BASS, HARMONIC, MELODIC) with per-layer voice and pattern
- Off-grid groove engine: DRAG, PUSH, ROLL, STUTTER, FRACTURE aesthetics with template system
- Voice bank: drum synthesis, mono-synth, additive, granular (BLOOM)
- Cross-cutting FX and spatial: PULVERIZE, HARDPAN, PINGPONG, AUTOPAN, FLUX modulator
- LLM bridge: natural-language → PatternPatch → validated apply
- Distributable as standalone + VST3/AU/LV2 on Linux/macOS/Windows

### Validated (Shipped)
None yet.

### Active (In Progress)
None yet.

### Planned (Next)
- F0: Scaffolding + spikes (CMake/JUCE setup, CI, thread-safety model, performance budget)
- F1: BEAT layer (drum voices, step sequencer, layer strip UI)
- F2: BASS layer + multi-pattern + save/load
- F3: Groove engine (DRAG/PUSH/ROLL/STUTTER/FRACTURE, template library)
- F4: HARMONIC + MELODIC layers
- F5: Cross-cutting FX & spatial
- F6: LLM bridge
- F7: Distribution (installers, presets, docs, beta)

### Out of Scope (v1)
- Full DAW arrangement timeline, audio recording of external sources
- Sampler workstation (multisampling, round-robin, SFZ)
- Streaming / networking / cloud presets / collaborative editing
- Notation/MIDI editor view (v2)
- Skin/theming system
- More than four layer types
- Plugin hosting inside LAYERZ
- Mobile/tablet build

## Constraints

### Technical Constraints
- Real-time audio safety: no allocations, no locks, no file I/O on audio thread
- All dependencies must be GPLv3-compatible (Faust runtime, JUCE GPLv3, nlohmann/json MIT, libcurl MIT)
- Cross-platform: Linux, macOS (incl. Apple Silicon), Windows
- Thread-safe Project model access (lock-free snapshot or message-passed — decided in F0.99b)
- Sub-buffer micro-timing accuracy with varying host buffer sizes (validated in F3.99)

### Business Constraints
- Solo developer bandwidth — phases are vertical slices for worst-case partial shipping
- GPLv3 license; no bundled samples without project ownership
- No telemetry, no phone-home

## Key Decisions

| Decision | Rationale | Date | Status |
|----------|-----------|------|--------|
| C++17 + JUCE (GPLv3 tier) | Cross-platform audio/plugin framework, GPLv3-compatible | Pre-init | Active |
| CMake (no Projucer) | Modern JUCE build path | Pre-init | Active |
| Faust for DSP-heavy components | BLOOM, PULVERIZE, additive authored in Faust; pre-compiled .cpp checked in | Pre-init | Active |
| JSON project model (.layerz) | Serializable, diffable, LLM-readable canonical state | Pre-init | Active |
| LLM bridge provider-neutral | PatternPatch protocol; Anthropic API as first target | Pre-init | Active |
| Two profiles: PLUGIN / STANDALONE | Different resource caps, same codebase | Pre-init | Active |
| Thread-safety model (TBD) | Lock-free snapshot vs message-passed — F0.99b spike | - | Pending spike |

## Success Metrics

| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| 30-sec beat with audible lurch/push/fracture | After F3, author records and recognizes feel | - | Not started |
| 2-min piece using all four layers | After F4, completable entirely in LAYERZ | - | Not started |
| Beta validation | 3–5 producers after F4 confirm concept before F7 | - | Not started |
| Plugin + standalone builds on Linux/macOS/Windows | All CI green | - | Not started |

## Tech Stack

| Layer | Technology | Notes |
|-------|------------|-------|
| Language | C++17 | JUCE-compatible baseline |
| Framework | JUCE (latest stable, GPLv3) | Audio, GUI, plugin formats, cross-platform |
| Build | CMake | Modern JUCE path; Projucer not used |
| DSP heavy | Faust (compiled to C++) | BLOOM, PULVERIZE, additive engine |
| DSP utility | juce::dsp | Filters, oscillators, FFT, delays, convolution, envelopes |
| GUI | JUCE Component graph, custom-painted | No HTML/web wrapper |
| Persistence | JSON (nlohmann/json or JUCE var/JSON) | .layerz, .layerzpreset, .layerztemplate |
| LLM bridge | HTTPS via juce::URL or libcurl | Provider-neutral; Anthropic API first |
| Testing | Catch2 + JUCE UnitTest | Unit + integration |
| Output formats | Standalone + VST3/AU/LV2 | All three via JUCE plugin client |
| Platforms | Linux, macOS (incl. Apple Silicon), Windows | CI builds all three |

## Links

| Resource | URL |
|----------|-----|
| Full project brief | ./PROJECT.md (root) |
| Full roadmap | ./ROADMAP.md (root) |
| Acceptance criteria | ./ACCEPTANCE.md (root) |

---
*Created: 2026-05-28*
*PROJECT.md — Updated when requirements or context change*
