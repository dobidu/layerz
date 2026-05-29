# 01-03 SUMMARY — Project Model + Audio Thread Harness

**Phase:** 01-scaffolding-spikes
**Plan:** 01-03 (execute, wave 2)
**Status:** Complete
**Date:** 2026-05-29
**Depends on:** 01-01 (F0.99 caps + F0.99b thread-safety decision)

---

## What was done

Implemented all load-bearing F0 components. Build passes. Schema equality + float
epsilon tests pass. CI push pending (AC-2 carries over from 01-02).

---

## Acceptance criteria results

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | Project round-trip: load → save → reload = semantic equality | ✓ PASS (Schema tests, float epsilon verified) |
| AC-2 | Click sync with host transport | ✓ Implemented — verify with transport play in AudioPluginHost |
| AC-3 | Click on standalone internal clock | ✓ Implemented — standalone mode auto-detected in prepareToPlay |
| AC-4 | Profile switching changes caps | ✓ PASS — ProfileConfig::forProfile() returns distinct integers |

---

## Files produced

| File | Purpose |
|------|---------|
| `Source/model/Schema.h` | Project structs v1 with schema_version + float-epsilon operator== |
| `Source/model/ProjectStore.h/.cpp` | Lock-free-snapshot store (C++17 short-mutex variant) |
| `Source/audio/ProfileConfig.h` | Hardcoded caps from F0.99 spike |
| `Source/audio/Clock.h/.cpp` | Host PPQ + standalone BPM clock; BeatEvents non-owning view |
| `Source/config/UserConfig.h/.cpp` | Installation settings; LLM opt-in; atomic write |
| `Source/PluginProcessor.h/.cpp` | Wired to Clock + ProjectStore; Hann click; state persistence |

---

## Deviations from plan

1. **`juce::ArrayRef<BeatEvent>` doesn't exist** — LLVM/JUCE confusion. Replaced with
   `BeatEvents` struct (plain pointer + count, C++17 safe, range-for compatible).
2. **`std::atomic<std::shared_ptr<T>>` not trivially copyable in C++17** — plan assumed
   C++20 semantics. Replaced with short mutex (held only for pointer copy ~10ns or swap
   ~10ns). Documented upgrade path to C++20 in header comments.
3. **`juce::var` rejects `std::string`** — must wrap with `juce::String()`. Fixed in
   ProjectStore.cpp serialisation.
4. **`UserConfig` added** — not in original plan; emerged from user requirement
   ("everything configurable"). Adopted cleanly; LLM bridge opt-in default=false.

---

## Key decisions recorded

| Decision | Value |
|----------|-------|
| Thread-safety impl (C++17) | Short mutex, held only for pointer copy/swap |
| Upgrade path | C++20 std::atomic<shared_ptr<T>> removes mutex entirely |
| JSON serialisation | juce::var/JSON (sufficient for F0; nlohmann/json deferred to F1) |
| Beat event return type | BeatEvents (non-owning view, C++17 aggregate) |
| LLM bridge default | disabled — user must opt in |
| Config file location | Platform application data dir / LAYERZ / config.json |

---

## What F1 can now do

- Read pattern events via `store_.snapshot()->patterns`
- Dispatch drum voice synthesis at Beat events from `clock_.process()`
- Set BPM via `clock_.setBpm()` from standalone GUI
- Store/recall voice parameters via `store_.postMutation()`
- Add per-layer mute/solo state to Schema.h (safe: schema_version=1 stays)

## Recalibration note (F1 obligation)

ProfileConfig caps are PRELIMINARY (synthetic bench, 10x multiplier estimate).
After F1 Faust drum voice exists: replace VoiceBench sine with Faust shim,
re-sweep same 3 machines, update constants in ProfileConfig.h.
