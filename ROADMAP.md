# LAYERZ — ROADMAP.md

## v1 target

LAYERZ v1 is a usable, distributable groovebox available as standalone application and as VST3/AU/LV2 plugin on Linux, macOS, and Windows. It produces music through four layers (BEAT, BASS, HARMONIC, MELODIC), each with its own voice palette and aesthetic toolkit. The off-grid groove engine (DRAG/PUSH/ROLL/STUTTER/FRACTURE), the granular voice (BLOOM), and the cross-cutting destructive/spatial FX (PULVERIZE, HARDPAN, PINGPONG, AUTOPAN, FLUX) are all functional. The LLM bridge accepts natural-language prompts and applies validated PatternPatches. A curated set of voice presets and aesthetic templates ships with the installer. The application is GPLv3 and distributed through GitHub Releases plus platform-specific channels (no app store gatekeeping in v1).

## v2 target (registered)

- Web port (TypeScript + Web Audio + AudioWorklet + Faust-compiled WASM).
- MIDI export from any pattern/layer.
- OSC I/O for external control and sync.
- Ableton Link support.
- "Perform mode" UI variant for live use.
- Additional voice types if v1's prove insufficient (FM, wavetable as candidates).

## v2 scope — Sample Banks (registered)

LAYERZ v1 notes sampling as "one of several voice sources (chop + play)." The full
sample bank system is registered for post-v1 development:

**Format**: A local directory structure with a `.layerzbank` JSON descriptor per bank:
```json
{
  "name": "My Bank",
  "version": 1,
  "voices": [
    { "name": "Kick 808", "file": "kick_808.wav", "tags": ["kick", "808"],
      "pitch_root": 36, "trim_start_ms": 0, "trim_end_ms": 0 }
  ]
}
```
- Banks are directories on the user's filesystem (no cloud, no telemetry)
- `.layerzbank` indexes samples, avoids rescanning on each load
- `pitch_root`: MIDI note the sample sounds "natural" at (enables pitch-correct playback)
- Tags enable filtering in the voice selector UI
- Format is GPLv3-compatible (JSON, self-describing, forward-compat via schema_version)

**Integration point**: VoiceBank → SampleVoice type (alongside DrumVoice, MonoSynth, Faust voices).
A SampleVoice reads from the pre-loaded bank, applies pitch shift and trim, renders to buffer.
Bank loading happens at project-load time on a background thread (never on audio thread).

**Roadmap placement**: Post-F4 (all four layers stable) — either as F4.5 or early F5.
The voice architecture (VoiceBank.h) is designed for this extension; adding a new voice
type does not change existing voices.

## Future ecosystem (not roadmapped)

- Hardware controller integration (Push-class devices, grid controllers).
- Companion mobile/tablet remote (read-only or limited).
- Pattern library / community preset sharing (via git, not a service).
- Plugin hosting inside LAYERZ (only if a clear need emerges).
- Integration with the author's other projects (Strudel pattern import, SuperCollider OSC bridge).

## Spike protocol

Spikes are time-boxed investigations that answer one specific question. They run **before** the phase they belong to and produce a written SUMMARY decision, not production code. Spike tasks use the suffix `99` within their phase (e.g. `F0.99`, `F3.99`). If the spike's answer changes the phase plan, the phase plan is updated before tasks start. The SUMMARY is committed to the repo under `docs/spikes/`.

Spikes for LAYERZ v1:

- **F0.99 — Performance budget.** Question: how many simultaneous voices and active grains can run at 256-sample buffer (typical DAW), 512-sample buffer, and 1024-sample buffer (typical standalone), on (a) the author's desktop Ryzen, (b) the author's MacBook Air M4, (c) a representative mid-range Linux laptop? Output: numbers that set the caps for `PROFILE_PLUGIN` and `PROFILE_STANDALONE`.
- **F0.99b — Thread-safety model.** Question: lock-free snapshot of `Project` per audio block, or message-passed mutations, or double-buffered state? Output: chosen pattern with a written rationale.
- **F3.99 — Groove engine accuracy under host clock.** Question: do sub-buffer micro-timing offsets remain perceptually accurate when the host buffer is large (512, 1024)? Does sample-accurate quantization suffice or do we need fractional-sample interpolation? Output: empirical measurement + decision.
- **F4.99 — Additive voice cost.** Question: how many partials per voice are affordable at our voice cap? Is the JUCE FFT path or a Faust additive bank cheaper? Output: chosen implementation.
- **F5.99 — PULVERIZE algorithm.** Question: grain-cloud-on-buffer vs FFT-phase-vocoder vs hybrid? Which gives the EPROM/Autechre character at acceptable CPU cost? Output: chosen algorithm + Faust skeleton.
- **F6.99 — PatternPatch schema and LLM prompting.** Question: what is the exact JSON schema for PatternPatch? What system prompt and examples yield reliable, validatable responses from current LLMs? Output: schema + prompt library + validation tests.

## Milestones

- **M1 — Skeleton runs.** Plugin and standalone open, produce sound, sync to host. (End of F1.)
- **M2 — Two-layer beat.** A user can make a BEAT+BASS pattern that loops with locked timing. (End of F2.)
- **M3 — Groove differential.** The off-grid engine produces audible, recognizable character. The 30-second validation recording is achievable. (End of F3.)
- **M4 — Four-layer music.** Full layer stack functional, the 2-minute piece is achievable. (End of F4.)
- **M5 — Aesthetic FX.** PULVERIZE, spatial FX, FLUX modulator integrated and performant. (End of F5.)
- **M6 — LLM bridge live.** Natural-language pattern generation/mutation works end-to-end with validated patches. (End of F6.)
- **M7 — Distributable v1.** Installers, presets, docs, onboarding. (End of F7.)

## Phases

### F0 — Scaffolding + spikes

**Goal.** Plugin and standalone build cleanly on all three OSes; audio thread produces a stable click synchronized to host transport (plugin) or internal clock (standalone); `Project` model exists as a JSON-backed struct with a thread-safe access pattern decided.

**Spikes.** F0.99 (performance budget), F0.99b (thread-safety model).

**Tasks (indicative).** JUCE project setup with CMake. Plugin formats VST3/AU/LV2 + standalone configured. CI for Linux/macOS/Windows. Stub `Project` model with JSON I/O. Audio thread harness with metronome click. Host sync (PPQ → internal step counter). Performance benchmark harness. Profile switching logic (`PROFILE_PLUGIN`/`PROFILE_STANDALONE`).

**Acceptance.** See ACCEPTANCE.md §F0.

### F1 — BEAT layer

**Goal.** A single BEAT layer plays a 16-step pattern with 3–4 drum voices (kick, snare, hat, perc) at locked-grid timing. Mixer with per-layer level. GUI shows the layer strip with steps, voice selector, level. The instrument makes a usable beat.

**Spikes.** None new; F0 spike results inform voice count caps.

**Tasks.** Drum voice synthesis (Faust): kick (sine + click), snare (noise + tone), hat (filtered noise), perc (configurable). Step sequencer reading from `Layer.events[]`. Step row UI Component. Voice parameter panel (slide-out). Per-layer level + mute/solo.

**Acceptance.** See ACCEPTANCE.md §F1.

### F2 — BASS layer + multi-pattern + chain

**Goal.** Add the BASS layer (mono-synth voice). Support multiple patterns per project and a basic chain (play pattern A then B then A). Two-layer music is possible. The user can save and reload a `.layerz` file.

**Tasks.** Mono-synth voice (Faust): saw/square/sub, envelope, filter with envelope, glide. Pattern list UI. Chain controls. Project save/load. Voice pitch input (BASS gets a pitched lane, not just steps). MIDI note input from host (for plugin mode) routed to BASS layer.

**Acceptance.** See ACCEPTANCE.md §F2.

### F3 — Groove engine (the differential)

**Goal.** DRAG, PUSH, ROLL, STUTTER, FRACTURE all functional and applicable to BEAT and BASS. Aesthetic knobs and template selector in the layer UI. First template library (5–8 templates per layer) curated. The 30-second validation recording is achievable.

**Spike.** F3.99 (groove engine accuracy under host clock).

**Tasks.** Aesthetic Resolver module (compile-time event transformer). DRAG distribution (per-hit offset with velocity-coupled attenuation). PUSH (symmetric to DRAG, positive offsets). ROLL (subdivision multiplier with envelope per re-trigger). STUTTER (gated repetition). FRACTURE (per-step probability, every-N-cycles conditions, Euclidean rotation operator). Deterministic-with-seed randomness for reproducibility. Template definition format + first template set. Morph knob (single-parameter deviation from template).

**Acceptance.** See ACCEPTANCE.md §F3.

### F4 — HARMONIC + MELODIC

**Goal.** Add HARMONIC layer (additive + BLOOM granular voice) and MELODIC layer (mono or limited poly, scale-constrained). Full four-layer music is possible. The 2-minute piece is achievable.

**Spike.** F4.99 (additive voice cost).

**Tasks.** Additive voice (Faust, partial bank with per-partial amp/freq). BLOOM voice (Faust granular: grain size, density, spray, position, pitch, window). MELODIC voice: shared mono-synth core with poly extension (cap 4). Scale/mode constraint module (notes snap to chosen scale). Motif mutation operators (transpose, invert, retrograde, length scale) — usable from UI buttons.

**Acceptance.** See ACCEPTANCE.md §F4.

### F5 — Cross-cutting FX & spatial

**Goal.** PULVERIZE, HARDPAN, PINGPONG, AUTOPAN, FLUX all integrated. Each is assignable per-layer or to master. The aesthetic stretches from "broken" to "destroyed" to "organic" as intended.

**Spike.** F5.99 (PULVERIZE algorithm).

**Tasks.** FX chain data model (list of FX per layer + master). PULVERIZE (Faust granular FX). HARDPAN (fixed pan with depth). PINGPONG (delay-coupled L/R alternation). AUTOPAN (LFO-driven pan). FLUX modulator (random-walk generator with rate, depth, smoothing). Modulation routing UI (drag-link from FLUX to a parameter, or assign explicitly).

**Acceptance.** See ACCEPTANCE.md §F5.

### F6 — LLM bridge

**Goal.** Natural-language prompts produce validated PatternPatches that the user previews and applies. The protocol is provider-neutral, tested against at least one major API.

**Spike.** F6.99 (PatternPatch schema + LLM prompt design).

**Tasks.** HTTPS client wrapper. Configuration UI for API endpoint + key (key stored locally only, never logged). System prompt builder (injects current `Project` summary, schema, glossary). Response parser. PatternPatch validator (schema + invariants). Preview UI (shows diff before commit). Apply / reject / re-prompt actions.

**Acceptance.** See ACCEPTANCE.md §F6.

### F7 — Distribution

**Goal.** LAYERZ v1 is downloadable, installable, and documented for users beyond the author.

**Tasks.** Final preset curation (voices and templates). README and user guide. In-app tour or first-run onboarding (minimal). Installer/packaging per OS (`.dmg`, `.deb`/`.AppImage`, `.exe`/MSI). GitHub Releases pipeline. License and attribution files. Beta with 3–5 producers; collect blockers; fix or document.

**Acceptance.** See ACCEPTANCE.md §F7.

## Phases for v2 (sketched, not committed)

- **F2.W — Web port skeleton.** Reuse `Project` model in TypeScript; compile Faust DSP to WASM; wire AudioWorklet; recreate UI in web tech.
- **F2.M — MIDI export.** Convert `Project` patterns to MIDI files at the rendering layer.
- **F2.O — OSC + Link.** Add OSC server/client and Ableton Link participant.
- **F2.P — Perform mode.** Alternate UI optimized for live performance.

## Risks to track in STATE.md

- **Real-time DSP performance, especially granular.** Mitigation: F0 and F5 spikes set hard caps; PULVERIZE quality gracefully degrades when caps hit.
- **Sub-tick groove accuracy with varying host buffers.** Mitigation: F3 spike measures; if sample-accuracy insufficient, add fractional interpolation.
- **GUI clutter pressure.** As features arrive, the "few parameters, easy to subvert" promise is the first thing to erode. Mitigation: every new control proposal goes through a "fits in current screen without nesting?" review.
- **LLM bridge reliability and latency.** Mitigation: all patches validated; failed/invalid patches surfaced with the raw text for debugging; never auto-apply.
- **Faust ↔ JUCE build complexity.** Mitigation: F0 establishes the build pipeline; all Faust modules are checked in as both `.dsp` source and pre-generated `.cpp` so contributors without Faust can still build.
- **Solo developer bandwidth.** Mitigation: phases are vertical slices — partial completion still yields a usable product. Worst case: F0–F3 still ships as a "groove-focused beat sketcher v0.5".
- **Scope creep from beta feedback at F7.** Mitigation: non-goals are explicit; F7 fixes blockers and documents the rest as v2.
- **License contamination.** Mitigation: dependency review at every phase; the only DSP entering the codebase is Faust-authored, JUCE-provided, or audited as permissive/GPL-compatible.
