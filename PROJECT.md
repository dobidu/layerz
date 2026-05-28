# LAYERZ — PROJECT.md

## What

LAYERZ is a layer-oriented groovebox built as a JUCE/C++ application that ships simultaneously as a standalone instrument and as a VST3/AU/LV2 plugin. Music is constructed bottom-up in four stacked layers — BEAT, BASS, HARMONIC, MELODIC — each with a deliberately narrow but deep set of sound generators and a focused aesthetic toolkit. The differentiator is not feature count: it is that the "off-the-grid" aesthetics that normally require hours of tweaking or multiple separate tools (lazy micro-timing, virtuosic rolls, fractured probabilistic patterns, aggressive granular destruction, organic spectral drift, hard-panned spatialization) are first-class citizens accessible behind templates and few parameters. An LLM bridge allows natural-language generation and mutation of patterns through a strict structured-patch protocol.

## Why

Existing tools force a trade-off LAYERZ rejects. Hardware grooveboxes (Circuit, Electribe) are immediate but their aesthetic palette is conservative — getting Dilla-style lurch or EPROM-style granular destruction out of them ranges from awkward to impossible. Full DAWs (Ableton, Bitwig, Reaper) can do all of it but only after the producer assembles a chain of plugins, MIDI effects, and habits — the path from "I want this feel" to "I am hearing this feel" is long. Live-coding environments (SuperCollider, Pure Data, TidalCycles) give total control at the cost of writing code instead of playing. LAYERZ targets the gap: the immediacy of a groovebox, the aesthetic reach of the DAW chain, none of the friction. Being free and open source removes commercial pressure to broaden the scope — the product can stay narrow on purpose.

## Who

- Primary: the producer/musician building beat-driven, texture-rich, "broken" or organic electronic music who wants to start from rhythm and stack outward without leaving the instrument.
- Self (N=1): the author and collaborators in the LAViD/computational-music orbit who currently assemble similar workflows by hand in SuperCollider/Pd/Strudel/Ableton.
- Live performers who want a single instrument that can do groove, texture, and aggressive FX without a laptop full of routed plugins.
- Curious DAW users who treat LAYERZ as a "character generator" — pattern source inside a larger session.

## Core architectural idea (canonical product)

The canonical product is the **`Project` model**: a serializable JSON document describing the entire musical state — tempo, patterns, layers, events, voice references, FX chains, modulation routings. Audio is one rendering of this model. Future MIDI/OSC export is another rendering. Future visualization is another. The LLM bridge does not synthesize audio or call the engine directly — it reads the `Project`, returns a `PatternPatch` (a diff over the model), and the engine applies the patch after validation. This is what survives v1 → v2 → v3 unchanged.

The shape (sketch, will firm up in F0 spike):

```
Project { tempo, swing_global, profile, scenes[], patterns[], voices[], modulations[] }
Pattern { id, length_steps, layers[], chain[] }
Layer { type: BEAT|BASS|HARM|MEL, voice_ref, events[], fx_chain[], aesthetics{} }
Event { step, velocity, micro_offset_ticks, pitch?, length?, params{} }
Aesthetics { drag, push, roll, stutter, fracture, bloom, pulverize, flux, ...}
```

## Non-goals (v1)

- **Not a full DAW.** No arrangement timeline beyond pattern chaining. No audio recording of external sources. No mixing of external buses. If the user needs that, they load LAYERZ in their DAW.
- **Not a sampler workstation.** Sampling is supported as one of several voice sources (chop + play), not as the central paradigm. No multisampling, no round-robin libraries, no SFZ.
- **No streaming/networking features.** No cloud presets, no collaborative editing, no peer audio transport. Users wanting collaboration use git or their DAW's session sharing.
- **No notation/MIDI editor view in v1.** Composition happens through steps and templates. MIDI export is registered for v2.
- **No "skin" or themeing system.** The visual identity is part of the product.
- **No more than four layer types.** Resisting "add a FX layer", "add a vocal layer", "add an automation layer" — those are functions of the existing four, not new layers.
- **No plugin hosting.** LAYERZ does not load third-party VST/AU inside itself. It is the instrument.
- **No mobile/tablet build in v1.** Desktop only.

## Stack

- **Language:** C++17 (JUCE-compatible baseline).
- **Framework:** JUCE (latest stable), used under GPLv3 (the free dual-license tier).
- **Build:** CMake (the modern JUCE path; Projucer not used).
- **DSP — heavy:** Faust, compiled to C++ and embedded. Granular synthesis (BLOOM), additive engine (HARMONIC voice), granular FX (PULVERIZE) are Faust-authored.
- **DSP — utility:** `juce::dsp` for filters, oscillators, FFT, delays, convolution, basic envelopes.
- **GUI:** JUCE Component graph, custom-painted. No HTML/web wrapper.
- **Persistence:** JSON (nlohmann/json or JUCE's `var`/`JSON`). Project files are `.layerz` (renamed `.json`).
- **LLM bridge:** HTTPS via `juce::URL` or libcurl, JSON over the wire. Provider-agnostic (Anthropic API as first target given author context, but the protocol is provider-neutral).
- **Testing:** Catch2 for unit tests; JUCE's `UnitTest` for integration where it pays off.
- **License:** GPLv3.

## Architecture

Seven components, communicating through the `Project` model as shared truth.

```
┌─────────────────────────────────────────────────────────────────┐
│                          GUI (JUCE)                             │
│   layer strips • template browser • template knobs • mixer      │
└──────────────┬─────────────────────────────────┬────────────────┘
               │                                 │
               ▼                                 ▼
   ┌──────────────────────┐         ┌───────────────────────────┐
   │   Project Model      │◄────────┤   LLM Bridge              │
   │   (canonical truth)  │         │   prompts ↔ PatternPatch  │
   └──────┬───────────────┘         └───────────────────────────┘
          │
          ▼
   ┌──────────────────────┐
   │  Pattern Compiler    │  →  schedules events per audio block,
   │  + Groove Engine     │     applying DRAG/PUSH/ROLL/STUTTER/
   │  + Aesthetic Resolver│     FRACTURE, with host PPQ alignment
   └──────┬───────────────┘
          │
          ▼
   ┌──────────────────────┐         ┌────────────────────────────┐
   │  Audio Engine        │◄────────┤  Voice Bank                │
   │  (block-rate render) │         │  drums • mono-synth •      │
   │                      │         │  additive • granular voice │
   └──────┬───────────────┘         └────────────────────────────┘
          │
          ▼
   ┌──────────────────────┐
   │  FX & Spatial        │  PULVERIZE • HARDPAN • PINGPONG •
   │  (cross-cutting)     │  AUTOPAN • FLUX modulator
   └──────┬───────────────┘
          │
          ▼
        Output (stereo bus to host or device)
```

Component responsibilities, exposures, and risks:

- **Project Model.** Holds the canonical JSON-shaped state in memory; exposes a thread-safe read API to the audio thread (via lock-free snapshots or double-buffering — to be decided in F0) and a mutating API to GUI and LLM. **Risk:** thread-safety. The audio thread cannot block on the GUI; updates must be atomic snapshots or message-passed.
- **Pattern Compiler + Groove Engine + Aesthetic Resolver.** Reads `Project`, produces a stream of timestamped events for each upcoming audio block. Applies DRAG/PUSH/ROLL/STUTTER/FRACTURE deterministically given a seed (so the user can re-roll or reproduce). Aligns to host PPQ when running as plugin; uses internal clock when standalone. **Risk:** sub-tick offset accuracy under varying host buffer sizes.
- **Voice Bank.** Stateless-as-possible voice generators (drum, mono-synth, additive, granular voice). Each voice is a class with `prepare()`, `process()`, and a parameter API. Voice parameters live in `Project.voices[]`. **Risk:** voice polyphony and granular grain count vs CPU budget.
- **Audio Engine.** The JUCE `AudioProcessor`'s `processBlock`. Pulls compiled events for the block, dispatches to voices, mixes layers, applies FX chain, writes to output. **Risk:** real-time safety (no allocations, no locks).
- **FX & Spatial.** Cross-cutting effects applied per-layer or to master. PULVERIZE is a Faust-based granular reprocessor; HARDPAN/PINGPONG/AUTOPAN are panning automators; FLUX is a modulator source that other parameters can subscribe to. **Risk:** PULVERIZE CPU cost and latency.
- **GUI.** JUCE Components. Each layer has identical anatomy (step row, voice selector, template chooser, aesthetic knobs); content differs. No tabs, no nested modals; everything reachable in one or two clicks. **Risk:** discoverability of "subversion" knobs without cluttering.
- **LLM Bridge.** Async HTTP client. Sends system prompt + schema + user intent; receives `PatternPatch`; validates against schema and invariants; applies on user approval (in v1, all LLM patches are previewed before commit). **Risk:** latency, response validity, provider lock-in if not careful.

## Session / data model

A **Session** is a single `Project` instance. There is no "song" above the session — pattern chaining and scene sequencing happen inside the project. A `Scene` is a named selection of which pattern each layer is currently playing; switching scenes is the macro-form control.

Files:

- `.layerz` — full project (JSON).
- `.layerzpreset` — voice/patch presets (JSON, single voice).
- `.layerztemplate` — aesthetic templates (JSON, parameter sets for DRAG/PUSH/etc).

## Modes / variants

LAYERZ has exactly two operational modes, with **mechanical differences** (not cosmetic):

| | `PROFILE_PLUGIN` (default when loaded in host) | `PROFILE_STANDALONE` (default when freestanding) |
|---|---|---|
| Clock | Follows host transport, BPM, PPQ | Internal clock, user-set BPM |
| Buffer size | Inherited from host (typically 64–512) | Chosen by user, larger allowed (e.g. 1024) |
| Max polyphony per voice | Conservative cap (calibrated by F0 spike) | Generous cap (calibrated by F0 spike) |
| Max active grains (BLOOM + PULVERIZE) | Conservative cap | Generous cap |
| GUI footprint | Resizable, designed to fit in plugin window | Resizable, designed to fill desktop window |
| Master output | Stereo to host bus | Stereo to system audio device |

The profile is read from the `Project` and overridable by the user (e.g. a power user on a fast machine may want plugin mode to use generous caps; this is one toggle, not a hidden setting). The user can swap profiles per session.

## Constraints

- **License:** GPLv3. All third-party dependencies must be GPLv3-compatible (Faust runtime: yes; JUCE under GPLv3: yes; nlohmann/json under MIT: yes; libcurl under MIT/X: yes).
- **OS:** Linux, macOS (including Apple Silicon), Windows. JUCE handles the cross-platform layer; CI builds all three.
- **Real-time audio safety:** No allocations, no locks, no file I/O on the audio thread. Enforced by code review and by audio-thread checks in debug builds.
- **No telemetry, no phone-home, no analytics.** Honest opt-in only if ever added.
- **No bundled samples that are not the project's own.** Avoids licensing tangles for OSS distribution.

## Future integrations (registered, not v1)

- Web port: TypeScript + Web Audio + AudioWorklet, with the same Faust DSP compiled to WASM. The `Project` model is reused as-is.
- MIDI export of patterns (the model already has all the data).
- OSC I/O for sync and external control.
- Ableton Link.
- "Perform mode" UI optimized for live use (large hit pads, scene morph).
- Additional voice types (FM, wavetable) — only if v1 voices prove insufficient in real use.
- Hardware controller mapping (push-style devices, Launchpad-style grids).
- Plugin hosting *inside* LAYERZ — explicitly not committed; may never happen.

## Honest limits

To document in the README so users come in calibrated:

- LAYERZ is a focused instrument, not a DAW. Some users will hit the wall and need to use it inside their DAW; that is fine and intended.
- Granular FX (PULVERIZE) is CPU-heavy. The plugin profile caps it conservatively; standalone unlocks it.
- The LLM bridge requires an API key from a provider; the user supplies it. It is also optional; LAYERZ is fully usable without LLM.
- Micro-timing offsets sub-buffer-size are quantized to sample accuracy; if the host runs at extremely large buffers (>1024), perceptual lurch may be slightly less precise.
- Project files are JSON, human-readable and diffable, but not backward-compatible across major versions without migration (a migration tool is in scope).

## First users and validation

The validation criteria (both must hold for v1 to ship):

- **After F3:** the author records a 30-second beat with audible micro-timing character ("lurch"/"push"/"fracture") and recognizes the feel as correct without editing outside LAYERZ.
- **After F4:** the author finishes a 2-minute musical piece entirely inside LAYERZ that uses all four layers.

Earlier validation gates exist per phase (see ACCEPTANCE.md). The deeper validation — "does this product have any reason to exist beyond N=1?" — is deferred to a small beta with 3–5 producers in the LAViD/local network after F4, before F7 distribution.

## UI design principles

LAYERZ's interface is part of the product, not skin over the engine. Principles to honor:

- **Layered anatomy mirrors the model.** The screen is vertically divided into the four layers (BEAT top, then BASS, HARMONIC, MELODIC), plus a master strip. Each layer is the same shape; content changes.
- **One screen, no tabs.** All primary operations are reachable in one click. Secondary (deep voice editing, modulation routing) lives in a slide-out or right-click panel — never a separate window.
- **Templates as fat buttons, subversion as one knob.** Each layer has a template row (5–8 templates per genre family) and a single "morph" knob that introduces controlled deviation. This is the "few-parameter / easily subverted" promise made architectural.
- **Color as identity, not decoration.** Each layer has one accent color (BEAT crimson, BASS amber, HARMONIC teal, MELODIC violet — proposal, revisitable). Used only on active states and the layer's own labels. Background is dark neutral.
- **Typography: monospaced for grid/values, sans for labels.** Step indices, velocity numbers, BPM, micro-offset values are mono. Layer names, template names, voice names are sans.
- **Animation only as feedback.** Play head, step trigger flash, knob value change. No idle motion, no decorative transitions.
- **Knobs over menus.** Wherever a value is continuous, it is a knob. Menus are only for discrete enumerated choices (voice type, template family).
- **Dark mode is the only mode.** No light theme in v1.
- **Resizable, with sane minimum.** Designed to be readable at ~900×600 (typical plugin window) and at full screen.
- **No animated splash, no logo modal.** App opens to the work surface.

A first sketch of the layout will be produced at F0 and refined through F4.

## Glossary

A new contributor needs to know:

- **Layer.** One of four horizontal strips: BEAT, BASS, HARMONIC, MELODIC. Each has a voice and a sequence of events.
- **Pattern.** A finite-length step sequence shared across layers (each layer has its own events within the pattern).
- **Scene.** A named selection of which pattern each layer plays. Switching scenes is the macro form-control.
- **Voice.** A sound generator instance, parameterized. Lives in the voice bank, referenced by a layer.
- **Event.** A single triggered note within a layer, with `step`, `velocity`, `micro_offset_ticks`, optionally `pitch`, `length`, and free `params`.
- **Aesthetic.** A per-layer set of groove/texture modifiers (DRAG, PUSH, ROLL, STUTTER, FRACTURE, BLOOM, FLUX) applied at compile time.
- **DRAG.** Negative micro-timing offset distributed per hit, with velocity attenuation. Produces "lazy" lurch.
- **PUSH.** Positive micro-timing offset, "rushing" feel.
- **ROLL.** Repeating subdivisions launched from a step (×2/×3/×4/×8). Virtuosic finger-drumming feel.
- **STUTTER.** Short gated repetition with re-trigger; produces IDM-style edits.
- **FRACTURE.** Probabilistic and conditional triggers (per-step probability, every-N-cycles conditions, Euclidean rotations). Produces broken/unpredictable patterns.
- **BLOOM.** Granular synthesis as a voice. Slow, cloud-like, organic.
- **PULVERIZE.** Granular reprocessing as an FX. Buffer freeze, smear, stutter, crush. Destructive.
- **FLUX.** Random-walk modulator. Can be assigned to partials of HARMONIC voice or to any continuous parameter; produces organic drift.
- **HARDPAN / PINGPONG / AUTOPAN.** Spatialization automators.
- **Profile.** The runtime resource configuration: `PROFILE_PLUGIN` (conservative) or `PROFILE_STANDALONE` (generous).
- **PatternPatch.** A structured diff over the Project model that the LLM produces and LAYERZ validates and applies.
- **Project.** The canonical model (`.layerz` file).
- **Template.** A named set of parameter values for a layer's aesthetic knobs. The user picks one and uses the "morph" knob to subvert it.
