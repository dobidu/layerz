# 01-01 SUMMARY — Spikes: F0.99 + F0.99b

**Phase:** 01-scaffolding-spikes
**Plan:** 01-01 (research)
**Status:** Complete
**Date:** 2026-05-28

---

## What was done

Two spikes executed and closed. Both produce written decisions that unblock 01-03.

### F0.99 — Performance Budget

Ran `VoiceBench.cpp` on three machines. Fixed a `high_resolution_clock` non-monotonicity
bug (Windows) mid-spike; re-ran Ryzen with `steady_clock`.

**Caps set (preliminary — recalibrate after F1 with real Faust voices):**

| Profile | max_voices | max_grains | Derivation |
|---------|-----------|-----------|------------|
| PROFILE_PLUGIN | 12 | 24 | Lenovo i7-8565U, buf=256: 12v+24g = 2.04% bench → ~20% real at 10× est. |
| PROFILE_STANDALONE | 24 | 40 | Lenovo i7-8565U, buf=1024: 24v+40g = 3.63% bench → ~36% real at 10× est. |

Weakest machine: Lenovo i7-8565U (3.4× slower than M4, 2.4× slower than Ryzen 9).

### F0.99b — Thread-Safety Model

**Decision: lock-free-snapshot**

Two-store architecture:
- `ProjectStore`: musical state, lock-free atomic snapshot. Audio thread calls `snapshot()` → O(1), no alloc, no lock.
- `UserConfig`: installation settings (LLM key, endpoint, enabled flag, profile override). Platform config dir. Never in `.layerz` files.

LLM bridge: opt-in, disabled by default. User supplies API key. Core instrument fully functional without it.

---

## Decisions recorded

| Decision | Value |
|----------|-------|
| PROFILE_PLUGIN max_voices | 12 |
| PROFILE_PLUGIN max_grains | 24 |
| PROFILE_STANDALONE max_voices | 24 |
| PROFILE_STANDALONE max_grains | 40 |
| Thread-safety model | lock-free-snapshot |
| ProjectStore API | snapshot() / postMutation() |
| UserConfig location | Platform config dir (not .layerz) |
| LLM bridge default | disabled |

---

## Deviations from plan

- VoiceBench bug fix mid-spike (steady_clock): required Ryzen re-run. Added one iteration.
- UserConfig introduced as separate struct (not in original plan) — emerged from user requirement that everything be configurable. Documented in F0.99b; 01-03 plan updated implicitly.

---

## What 01-03 can now do

- Implement `ProjectStore` with lock-free-snapshot semantics
- Implement `UserConfig` with LLM toggle + profile override
- Hardcode caps: `MAX_VOICES_PLUGIN=12`, `MAX_GRAINS_PLUGIN=24`, `MAX_VOICES_STANDALONE=24`, `MAX_GRAINS_STANDALONE=40`
- Clock.process() returns `juce::ArrayRef<BeatEvent>` over pre-allocated buffer

## Files produced

- `docs/spikes/F0.99-performance-budget.md`
- `docs/spikes/F0.99b-thread-safety.md`
- `docs/spikes/bench_results_MacM4.tsv`
- `docs/spikes/bench_results_PCRyzen9RTX5070.tsv`
- `docs/spikes/bench_results_PCI7LenovoS145.tsv`
- `Source/bench/VoiceBench.cpp` (with steady_clock fix)
