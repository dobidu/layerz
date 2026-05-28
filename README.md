# LAYERZ

**A layer-oriented groovebox for desktop and DAW.**

LAYERZ is a free, open-source instrument built with JUCE/C++ that ships as both a standalone application and as a VST3/AU/LV2 plugin. Music is built bottom-up through four stacked layers — BEAT, BASS, HARMONIC, MELODIC — each with a deliberately narrow but deep palette of sound generators and a focused aesthetic toolkit.

The differentiator is not feature count. The off-grid aesthetics that normally require hours of tweaking — Dilla-style lurch, IDM stutter, granular destruction, organic spectral drift, hard-panned spatialization — are first-class citizens here, accessible behind templates and a single morph knob. An optional LLM bridge lets you describe pattern mutations in plain language.

**Status:** Pre-release. F0 (scaffolding + spikes) in progress. See [Roadmap](#roadmap).

---

## Why LAYERZ exists

Existing tools force a trade-off LAYERZ rejects:

| Tool | Immediate | Aesthetic reach | Friction |
|------|-----------|----------------|---------|
| Hardware grooveboxes (Circuit, Electribe) | ✓ | ✗ Conservative palette | Low |
| Full DAWs (Ableton, Bitwig) | ✗ | ✓ | High — assemble plugin chains first |
| Live-coding (SuperCollider, TidalCycles) | ✗ | ✓ | High — write code, not music |
| **LAYERZ** | **✓** | **✓** | **Low** |

LAYERZ targets the gap: the immediacy of a groovebox, the aesthetic reach of a DAW chain, none of the assembly friction. Being GPLv3 and free removes commercial pressure to broaden the scope — the instrument stays narrow on purpose.

---

## Four layers, one workflow

Every project is built through the same four layers, in order:

```
┌─────────────────────────────────────────────────────────────────┐
│  BEAT      — rhythm and percussion                              │
│  BASS      — harmonic foundation, mono-synth                   │
│  HARMONIC  — texture and colour, additive + granular voice      │
│  MELODIC   — motif and movement, scale-constrained              │
└─────────────────────────────────────────────────────────────────┘
```

Each layer has the same anatomy: a step sequencer, a voice, a template picker, and a morph knob. Content changes; the interface does not.

---

## Aesthetic toolkit

The groove engine applies per-layer transformations at compile time, before audio rendering:

| Aesthetic | Effect |
|-----------|--------|
| **DRAG** | Negative micro-timing offsets per hit, velocity-coupled. Produces lazy lurch. |
| **PUSH** | Positive micro-timing offsets. Rushing feel. |
| **ROLL** | Subdivision multipliers (×2/×3/×4/×8) with per-re-trigger envelope. |
| **STUTTER** | Gated short repetitions. IDM-style edits. |
| **FRACTURE** | Per-step probability, every-N-cycles conditions, Euclidean rotation. |

Cross-cutting FX (applied per-layer or to master):

| FX | Effect |
|----|--------|
| **BLOOM** | Granular synthesis voice — slow, cloud-like, organic. |
| **PULVERIZE** | Granular reprocessing FX — buffer freeze, smear, stutter, crush. Destructive. |
| **HARDPAN** | Fixed stereo placement with depth control. |
| **PINGPONG** | Delay-coupled alternating L/R panning. |
| **AUTOPAN** | LFO-driven pan oscillation. |
| **FLUX** | Random-walk modulator assignable to any continuous parameter. Organic drift. |

---

## Templates and the morph knob

Each layer ships with 5–8 named templates per genre family (e.g. "Lurch Soul", "Fracture Grid", "Bloom Wash"). A single morph knob introduces controlled deviation from the template across all aesthetic parameters simultaneously. This is the "few parameters, easily subverted" promise made architectural.

---

## LLM bridge

An optional HTTPS bridge sends the current project context and a natural-language prompt to a configured LLM endpoint, receives a `PatternPatch` (a structured JSON diff over the project model), validates it, and presents it for preview before applying. The LLM never calls the audio engine directly — it speaks only the `Project` model. The bridge is provider-neutral; Anthropic's API is the first target.

---

## The `Project` model

Everything LAYERZ produces is a serializable JSON document:

```json
{
  "schema_version": 1,
  "tempo": 120.0,
  "swing_global": 0.0,
  "profile": "PLUGIN",
  "patterns": [
    {
      "id": "pattern-1",
      "length_steps": 16,
      "layers": [
        {
          "type": "BEAT",
          "voice_ref": "kick-01",
          "events": [
            { "step": 0, "velocity": 1.0, "micro_offset_ticks": 0 }
          ],
          "aesthetics": { "drag": 0.3, "push": 0.0 }
        }
      ]
    }
  ]
}
```

Project files use the `.layerz` extension. They are human-readable, version-controlled, and directly consumed by the LLM bridge. Audio is one rendering of this model; future MIDI export is another.

---

## Plugin modes

LAYERZ has two operational profiles with mechanical differences:

| | `PROFILE_PLUGIN` | `PROFILE_STANDALONE` |
|---|---|---|
| Clock | Host transport (PPQ) | Internal clock |
| Buffer size | Host-set (typically 64–512) | User-set (up to 1024+) |
| Max polyphony | Conservative (set by F0.99 spike) | Generous (set by F0.99 spike) |
| Max grains | Conservative | Generous |
| GUI footprint | Resizable, plugin-window optimised | Resizable, full-screen optimised |
| Output | Stereo to host bus | Stereo to system audio |

The profile is stored in the project file and is overridable per session.

---

## UI design principles

- **One screen, no tabs.** All primary operations reachable in one click.
- **Templates as buttons, subversion as one knob.** No parameter walls.
- **Color as identity, not decoration.** BEAT: crimson. BASS: amber. HARMONIC: teal. MELODIC: violet.
- **Monospaced for values, sans for labels.**
- **Animation only as feedback.** No idle motion.
- **Knobs over menus** for continuous values.
- **Dark mode only.**
- **Resizable with sane minimum** (~900×600 for plugin windows).
- **No animated splash. No logo modal.** Opens to the work surface.

---

## Non-goals (v1)

These were considered and deliberately excluded. They are not bugs, they are constraints:

- No arrangement timeline beyond pattern chaining. Load LAYERZ in your DAW if you need one.
- No sampler workstation. Sampling is one voice source among several, not the central paradigm.
- No streaming, cloud presets, or collaborative editing.
- No notation/MIDI editor view (MIDI export is v2).
- No skin or theming system.
- No more than four layer types.
- No plugin hosting inside LAYERZ.
- No mobile or tablet builds.
- No telemetry, no phone-home, no analytics.

---

## Building

### Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | ≥ 3.22 | No Projucer |
| C++ compiler | C++17 | GCC 11+, Clang 14+, MSVC 2022 |
| JUCE | 8.x (auto-fetched) | GPLv3 tier via FetchContent |
| Faust | ≥ 2.60 | For DSP modules (F4+); pre-compiled `.cpp` checked in |
| nlohmann/json | ≥ 3.11 (auto-fetched) | MIT licence |
| Catch2 | ≥ 3.x (auto-fetched) | Testing |

JUCE, nlohmann/json, and Catch2 are fetched automatically by CMake on first configure. No manual installation required.

### Linux additional packages

```bash
sudo apt-get install \
  libasound2-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libfreetype6-dev libgl1-mesa-dev liblv2-dev \
  pkg-config
```

### Build

```bash
git clone https://github.com/YOUR_ORG/layerz.git
cd layerz
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

First configure downloads JUCE (~1.5GB). Subsequent builds use the CMake cache.

Build artifacts:

| Platform | Format | Location |
|----------|--------|----------|
| All | Standalone | `build/LAYERZ_artefacts/Release/Standalone/` |
| All | VST3 | `build/LAYERZ_artefacts/Release/VST3/` |
| macOS | AU | `build/LAYERZ_artefacts/Release/AU/` |
| Linux | LV2 | `build/LAYERZ_artefacts/Release/LV2/` |

### CI

GitHub Actions builds all three platforms on every push to `main`. See `.github/workflows/ci.yml`. FetchContent output is cached keyed on `CMakeLists.txt` hash.

---

## Performance benchmark (VoiceBench)

Before starting implementation, run the standalone CPU benchmark on your target machines to set voice and grain caps for `PROFILE_PLUGIN` and `PROFILE_STANDALONE`:

```bash
clang++ -O3 -DNDEBUG -std=c++17 Source/bench/VoiceBench.cpp -o VoiceBench
./VoiceBench > bench_results_MACHINENAME.tsv
```

Output columns: `buffer_size | n_voices | n_grains | peak_cpu | mean_cpu | stddev_cpu | overruns`

Runs 5 independent 10-second windows per configuration for statistical validity. Identify the highest voice+grain count where `mean_cpu < 25%` at `buffer=256` (plugin cap) and `mean_cpu < 40%` at `buffer=1024` (standalone cap). The weakest machine's result at `buffer=256` sets the plugin cap.

Results feed into `docs/spikes/F0.99-performance-budget.md`.

---

## Project structure

```
layerz/
├── CMakeLists.txt              # Root CMake: JUCE, plugin targets, CI config
├── Source/
│   ├── PluginProcessor.h/.cpp  # JUCE AudioProcessor (stub → grows phase by phase)
│   ├── PluginEditor.h/.cpp     # JUCE AudioProcessorEditor (stub → GUI)
│   ├── model/                  # Project model (Schema.h, Project.h/.cpp) — added F0
│   ├── audio/                  # Clock, ProfileConfig, AudioEngine — added F0/F1
│   └── bench/
│       └── VoiceBench.cpp      # Standalone CPU benchmark (F0.99 spike tool)
├── docs/
│   └── spikes/                 # Written spike decisions (F0.99, F0.99b, F3.99, …)
├── .github/
│   └── workflows/ci.yml        # 3-OS matrix CI (ubuntu, macos, windows)
├── .paul/                      # PAUL project management state
│   ├── PROJECT.md              # Requirements, constraints, decisions
│   ├── ROADMAP.md              # Phase breakdown
│   ├── STATE.md                # Current position, loop state, session continuity
│   └── phases/                 # Per-phase plan and summary files
├── PROJECT.md                  # Full project brief (architecture, rationale, glossary)
├── ROADMAP.md                  # Full phase roadmap with milestones and spikes
├── ACCEPTANCE.md               # Given/When/Then acceptance criteria per phase
└── USAGE.md                    # Toolchain setup (PAUL + graphify + caveman)
```

---

## Roadmap

| Phase | Name | Milestone | Status |
|-------|------|-----------|--------|
| F0 | Scaffolding + Spikes | — | In progress |
| F1 | BEAT layer | M1: Skeleton runs | Planned |
| F2 | BASS + multi-pattern | M2: Two-layer beat | Planned |
| F3 | Groove engine | M3: Groove differential | Planned |
| F4 | HARMONIC + MELODIC | M4: Four-layer music | Planned |
| F5 | Cross-cutting FX & spatial | M5: Aesthetic FX | Planned |
| F6 | LLM bridge | M6: LLM bridge live | Planned |
| F7 | Distribution | M7: Distributable v1 | Planned |

**Validation gates (subjective, author-signed):**
- After F3: record a 30-second beat with audible lurch/push/fracture character. Confirm the feel is correct.
- After F4: produce a 2-minute piece using all four layers entirely in LAYERZ.
- After F4, before F7: beta with 3–5 producers in the LAViD/local network.

Full phase details, spike protocol, and risk register: [`ROADMAP.md`](ROADMAP.md).

---

## Honest limits

- LAYERZ is a focused instrument, not a DAW. Some users will need to run it inside their DAW — that is intended and fine.
- PULVERIZE (granular FX) is CPU-heavy. The plugin profile caps it conservatively; standalone unlocks it.
- The LLM bridge requires an API key from a provider. It is entirely optional — LAYERZ is fully usable without it.
- Micro-timing offsets sub-buffer-size are quantized to sample accuracy. At very large host buffers (>1024), perceptual precision may be slightly reduced.
- Project files (`.layerz`) are JSON. Human-readable and diffable, but not guaranteed backward-compatible across major versions without a migration tool.

---

## Glossary

| Term | Meaning |
|------|---------|
| **Layer** | One of four horizontal strips: BEAT, BASS, HARMONIC, MELODIC. Each has a voice and a step sequence. |
| **Pattern** | A finite-length step sequence shared across layers (each layer has its own events within the pattern). |
| **Scene** | A named selection of which pattern each layer plays. Switching scenes is the macro form-control. |
| **Voice** | A sound generator instance. Lives in the voice bank, referenced by a layer. |
| **Event** | A single triggered note: `step`, `velocity`, `micro_offset_ticks`, optionally `pitch`, `length`, `params`. |
| **Aesthetic** | A per-layer groove/texture modifier applied at compile time: DRAG, PUSH, ROLL, STUTTER, FRACTURE. |
| **Template** | A named set of aesthetic parameter values. Picked from a row of buttons; subverted by the morph knob. |
| **PatternPatch** | A structured JSON diff over the Project model produced by the LLM bridge and validated before apply. |
| **Profile** | Runtime resource configuration: `PROFILE_PLUGIN` (conservative) or `PROFILE_STANDALONE` (generous). |
| **Project** | The canonical model (`.layerz` file). Everything else is a rendering of it. |
| **DRAG** | Lazy-lurch: negative micro-timing offset with velocity-coupled attenuation. |
| **PUSH** | Rushing: positive micro-timing offset, mirror of DRAG. |
| **ROLL** | Subdivision multiplier (×2/×3/×4/×8) with per-re-trigger velocity envelope. |
| **STUTTER** | Gated short repetitions — IDM-style edits. |
| **FRACTURE** | Probabilistic and conditional triggers: per-step probability, every-N-cycles conditions, Euclidean rotation. |
| **BLOOM** | Granular synthesis voice: slow, cloud-like, organic. |
| **PULVERIZE** | Granular reprocessing FX: buffer freeze, smear, stutter, crush. Destructive. |
| **FLUX** | Random-walk modulator assignable to any continuous parameter. Organic drift, not LFO. |

---

## Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| Framework | JUCE (latest stable, GPLv3 tier) |
| Build | CMake ≥ 3.22 (no Projucer) |
| DSP — heavy | Faust (compiled to C++, embedded) |
| DSP — utility | `juce::dsp` |
| GUI | JUCE Component graph, custom-painted |
| Persistence | JSON via nlohmann/json — `.layerz`, `.layerzpreset`, `.layerztemplate` |
| LLM bridge | HTTPS via `juce::URL` or libcurl, provider-neutral |
| Testing | Catch2 + JUCE `UnitTest` |
| CI | GitHub Actions (ubuntu, macos, windows) |
| Platforms | Linux, macOS (incl. Apple Silicon), Windows |
| Output formats | Standalone + VST3 + AU (macOS) + LV2 (Linux) |

---

## Development workflow

This project uses [PAUL](https://github.com/paul-framework/paul) for systematic phase-based development. The workflow is:

```
/paul:plan   →   /paul:apply   →   /paul:unify
```

State is tracked in `.paul/STATE.md`. Plans live in `.paul/phases/`. Each phase closes with a SUMMARY that feeds the next plan.

For new contributors: read `PROJECT.md` (architecture and rationale), `ROADMAP.md` (what each phase delivers), and `ACCEPTANCE.md` (how correctness is proven) before opening a PR.

---

## License

GPLv3. See `LICENSE` (to be added at F7).

All dependencies are GPLv3-compatible:
- JUCE under GPLv3 (free tier)
- Faust runtime — GPLv3-compatible
- nlohmann/json — MIT
- libcurl — MIT/X

No bundled samples except those authored for this project. No telemetry. No network calls except the user-configured LLM endpoint.

---

*LAYERZ is a focused instrument. It does one thing and goes deep on it.*
