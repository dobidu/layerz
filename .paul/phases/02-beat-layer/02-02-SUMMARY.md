# 02-02 SUMMARY — BEAT Layer UI

**Phase:** 02-beat-layer
**Plan:** 02-02 (execute, wave 2)
**Status:** Complete
**Date:** 2026-05-29

---

## Acceptance criteria results

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | Step toggle mutates Project + audio changes | ✓ PASS |
| AC-2 | Level + mute take effect immediately | ✓ PASS |
| AC-3 | param1 slider changes voice character | ✓ PASS |
| AC-4 | RT safety during UI interaction (60s) | ✓ PASS |

---

## Files produced

| File | Purpose |
|------|---------|
| `Source/gui/StepRow.h/.cpp` | 16 crimson step buttons, optimistic update, step position indicator (3px bar) |
| `Source/gui/VoiceParamPanel.h/.cpp` | Level/mute/param1 controls with juce::dontSendNotification |
| `Source/gui/BeatLayerStrip.h/.cpp` | 4 drum rows, 30fps timer, alternating row bg |
| `Source/PluginEditor.h/.cpp` | BeatLayerStrip + transport bar; juce::Timer syncs play button state |

---

## Deviations and post-checkpoint fixes

1. **kBeatStripH=280 clipped bottom row** — header (24px) not counted. Fixed: 304px.
2. **Step indicator hidden under buttons** — buttons covered full row height. Fixed: leave 4px gap.
3. **Play button state unsync** — auto-play in prepareToPlay left button showing PLAY while clock played. Fixed: LayerzEditor inherits Timer (10fps), syncs toggle state via dontSendNotification.
4. **Mono output** — voices wrote only to ch0. Fixed: copy ch0→ch1 in processBlock.
5. **Hat sounded harsh/digital** — HP-filtered noise has wrong character. Fixed: ring modulation of two inharmonic sines (f1=8kHz, f2=10.3kHz) — metallic ring quality.
6. **Standalone detection race** — isStandaloneMode() called before prepareToPlay. Fixed: use wrapperType == wrapperType_Standalone in PluginEditor constructor.

---

## Key decisions

| Decision | Rationale |
|----------|-----------|
| Hat = ring-mod oscillators not HP noise | HP noise too digital; inharmonic sine products create realistic metallic character |
| juce::dontSendNotification everywhere in updateFromSnapshot | Prevents infinite postMutation loops (audit M6/M7) |
| Play button synced via Timer not callback | prepareToPlay runs after editor construction; timer handles the async gap |

---

## Next: /paul:unify
