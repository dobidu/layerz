# LAYERZ — USAGE.md

How to install and operate the PAUL + graphify + caveman toolchain on the LAYERZ project.

## The four artifacts

This repository ships four documents that drive the PAUL workflow:

- **`PROJECT.md`** — context for `/paul:init`. Defines what LAYERZ is, why it exists, who it serves, the canonical product (the `Project` model), non-goals, stack, architecture, modes, constraints, future integrations, honest limits, validation criteria, UI principles, and glossary.
- **`ROADMAP.md`** — phases F0 through F7, milestones M1 through M7, the spike protocol with specific spikes (F0.99, F3.99, F4.99, F5.99, F6.99), v2 sketch, and risks to track in `STATE.md`.
- **`ACCEPTANCE.md`** — Given/When/Then per phase. Seeds for `/paul:plan` invocations. Includes criteria that prove the `PROFILE_PLUGIN` / `PROFILE_STANDALONE` distinction is mechanical (not cosmetic) and that the canonical product abstraction holds.
- **`USAGE.md`** — this file.

## Install order

Install the toolchain in this order. Each tool is independent, but PAUL's experience is best when caveman is already installed (token savings) and graphify is added once the project has structure to graph.

```
1. caveman           (token-compression slang, project-wide)
2. PAUL              (the systematic-coding agent)
3. graphify          (add after Phase 1 once code exists)
```

For the author (familiar with all three), this is muscle memory. For collaborators new to PAUL, the README of each tool is the source of truth; do not paraphrase their install steps here.

## Command flow for the first phases

After cloning the repo and ensuring the four artifacts are present:

```
/paul:init
```

PAUL reads `PROJECT.md`, builds its mental model of LAYERZ, and confirms it understood. Correct any drift before proceeding.

```
/paul:plan F0
```

PAUL produces a detailed plan for F0 (scaffolding + spikes), referencing `ROADMAP.md` for tasks and `ACCEPTANCE.md` §F0 for outcomes. The plan is reviewed before any code is written.

```
/paul:spike F0.99
/paul:spike F0.99b
```

Both spikes are run before F0 tasks. Each produces `docs/spikes/F0.99.md` (and `F0.99b.md`) containing the question, methods, measurements, and decision. The performance budget spike sets the voice/grain caps for `PROFILE_PLUGIN` and `PROFILE_STANDALONE`. The thread-safety spike chooses between snapshot, message-passing, or double-buffering for `Project` access from the audio thread.

```
/paul:work F0
```

PAUL executes the F0 tasks. The acceptance criteria from §F0 of `ACCEPTANCE.md` are the gates.

After F0 completes, install graphify:

```
graphify init
```

graphify now has real source files to graph. From here on, the loop is:

```
/paul:plan F<n>
/paul:spike F<n>.99    # if present
/paul:work F<n>
/paul:verify F<n>      # against ACCEPTANCE.md §F<n>
```

## Spike protocol reminder

Spikes use suffix 99 within their phase. They run **before** the phase. They are time-boxed and their output is a written decision, not code. Acceptance for a spike phase is not Given/When/Then — it is the existence of the SUMMARY in `docs/spikes/`. If the SUMMARY's decision changes the phase plan, the plan is updated before any task begins.

LAYERZ v1 spikes:

- F0.99 — Performance budget (voice/grain caps per profile).
- F0.99b — Thread-safety model for the `Project`.
- F3.99 — Groove engine accuracy under varying host buffer sizes.
- F4.99 — Additive voice cost (partials vs polyphony).
- F5.99 — PULVERIZE algorithm choice (grain cloud vs phase vocoder vs hybrid).
- F6.99 — PatternPatch schema and LLM prompt design.

## Risks summary

From `ROADMAP.md`, tracked in `STATE.md` as the project progresses:

- Real-time DSP performance, especially granular (PULVERIZE, BLOOM).
- Sub-tick groove accuracy with large host buffers.
- GUI clutter as features arrive ("few parameters, easy to subvert" is fragile).
- LLM bridge reliability and latency; invalid responses must never silently corrupt the project.
- Faust ↔ JUCE build pipeline complexity.
- Solo developer bandwidth — vertical-slice phases mitigate by leaving usable partial products at every step.
- License contamination — every dependency is reviewed for GPLv3 compatibility.

## What is deliberately out of scope

These were considered and rejected for v1. They are not bugs, they are choices. Pushing back on attempts to add them mid-project is part of the discipline:

- No arrangement timeline beyond pattern chaining and scenes.
- No external audio recording or mixing.
- No cloud, no telemetry, no network features other than the user-configured LLM endpoint.
- No multisampling, no SFZ, no sample libraries beyond what the author bundles as bare-bones presets.
- No notation editor, no MIDI editor view (MIDI **export** is v2).
- No skinning or theming.
- No mobile/tablet builds.
- No plugin hosting inside LAYERZ.
- More than four layer types — resist additions like "FX layer", "vocal layer", "automation layer".

## A note on tone and toolchain familiarity

The author has run BMAD/PAUL many times. PAUL prompts that try to re-explain the framework can be safely interrupted with "skip the framework recap, proceed". The artifacts in this repo are dense by design — they are sized for someone who already knows how to read PAUL-shaped documents. Collaborators new to PAUL should read its docs once before reading these.

## After v1 ships

`v2` is sketched in `ROADMAP.md` and not committed. When v1 ships, the v2 decisions reopen: web port, MIDI/OSC export, Ableton Link, perform mode, additional voices. The `Project` model is the same; the renderings multiply.
