# 01-02 SUMMARY — CMake/JUCE Setup + CI

**Phase:** 01-scaffolding-spikes
**Plan:** 01-02 (execute)
**Status:** Complete
**Date:** 2026-05-29

---

## What was done

CMakeLists.txt, stub plugin source, and GitHub Actions CI pipeline created and
verified working on macOS. Three build fixes required during execution.

---

## Acceptance criteria results

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | VST3 + Standalone build on macOS | ✓ PASS |
| AC-2 | CI matrix green (ubuntu/macos/windows) | Pending — check GitHub Actions |
| AC-3 | VST3 loads in host without crash | ✓ PASS — verified in JUCE AudioPluginHost |

---

## Build fixes applied (deviations from plan)

Three issues discovered during the build; all fixed and committed:

1. **`LANGUAGES CXX` only** — JUCE requires C language too. Added `LANGUAGES C CXX`.
2. **LV2URI missing** — JUCE warned about missing `urn:` prefix. Added `LV2URI "urn:layerz:plugin"`.
3. **`-Werror` scoped incorrectly** — `target_compile_options` on the LAYERZ target applies
   to JUCE module sources too (which have unfixable warnings in 8.0.4). Replaced with
   `set_source_files_properties` scoped to `Source/*.cpp` only.
4. **`JUCE_DISPLAY_SPLASH_SCREEN=1`** — JUCE 8 removed splash entirely; flag generates a
   `#pragma message` warning. Removed.
5. **`processorRef` unused** — `-Wunused-private-field` in PluginEditor.h. Removed field;
   stub accesses processor via `getAudioProcessor()` when needed in F1.
6. **`g.setFont(float)` deprecated** — JUCE 8 deprecates `Graphics::setFont(float)`.
   Replaced with `juce::Font(juce::FontOptions{}.withHeight(18.0f))`.

---

## Files produced

- `CMakeLists.txt` — JUCE 8.0.4 via FetchContent, VST3/AU/LV2/Standalone, AudioPluginHost optional target
- `Source/PluginProcessor.h/.cpp` — stub AudioProcessor (silence out)
- `Source/PluginEditor.h/.cpp` — stub editor (900×600, dark grey, "LAYERZ" centred)
- `.github/workflows/ci.yml` — 3-OS matrix with FetchContent cache

## Key decisions

- JUCE 8.0.4 — pinned. Known minor bug in `juce_NSViewComponentPeer_mac.mm` (unused var warning);
  mitigated by scoping -Werror to project sources only. Update tag when 8.0.5+ available.
- AudioPluginHost added as `EXCLUDE_FROM_ALL` target for local VST3 testing.
