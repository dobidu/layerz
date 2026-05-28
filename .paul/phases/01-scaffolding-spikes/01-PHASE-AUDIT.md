# Enterprise Plan Audit Report

**Plans:** 01-01-PLAN.md, 01-02-PLAN.md, 01-03-PLAN.md
**Audited:** 2026-05-28
**Verdict:** Conditionally acceptable — applied fixes make plans enterprise-ready

---

## 1. Executive Verdict

The three plans are **conditionally acceptable**. None contained fundamental architectural flaws — the scope, sequencing, and dependency structure are sound. However, four must-have gaps would have caused real failures: a legally non-compliant JUCE build, a CI pipeline that cannot meet its own timing AC, production data corruption on interrupted saves, and migration-impossible schema files. All four were applied. The plans are now approvable.

Would I sign off on this plan set post-audit? Yes — with the applied changes in place.

---

## 2. What Is Solid

- **Spike-before-implement sequence** is correct. Blocking 01-03 on 01-01 decisions is the right call — implementing thread-safety without a chosen pattern produces code that must be rewritten.
- **Wave 1 parallelism** (01-01 + 01-02 concurrent) is justified: CMake/JUCE setup genuinely does not need spike answers. This saves real calendar time.
- **Boundary sections** in all three plans are specific and non-overlapping. No file is touched by two plans.
- **Acceptance criteria** map directly to ACCEPTANCE.md Given/When/Then — no invented or missing coverage.
- **Real-time safety discipline**: the no-allocation/no-lock constraint appears in every audio-thread task. This is correct and must stay.
- **Faust pre-compilation strategy** (checked-in .cpp so contributors without Faust can build) is referenced from PROJECT.md and correctly deferred — not dragged into F0 scope.

---

## 3. Enterprise Gaps Identified

### 01-01 (Spikes)

**G1 [Must-Have] Benchmark statistical invalidity**: Single 10-second run per config. A background process (browser tab, package manager, Spotlight index) can inflate CPU% by 10–30%. Caps derived from a single run are noise, not signal. ProfileConfig.h values will be hardcoded from this spike forever — if the measurement is wrong, every voice cap in the product is wrong.

**G2 [Must-Have] `-O2` is not Release**: LLVM/GCC Release mode uses `-O3` with autovectorization and loop unrolling. `-O2` under-estimates what the shipped binary can do, producing artificially conservative caps.

**G3 [Strongly Recommended] No stddev in output**: The human reviewing the TSV cannot assess measurement reliability without variance data. If stddev_cpu > mean_cpu × 0.2, the measurement is too noisy to trust.

**G4 [Strongly Recommended] No weakest-machine policy in the checkpoint**: The human-action step tells users to "identify the voice+grain count" but not which machine's number becomes the cap. This produces ambiguity in the spike document.

### 01-02 (CMake + CI)

**G5 [Must-Have] JUCE GPLv3 splash screen missing**: JUCE's free tier requires `JUCE_DISPLAY_SPLASH_SCREEN=1`. Without it, the binary is non-compliant with JUCE's license terms from the first build. This is not a style issue — it is a legal obligation.

**G6 [Must-Have] FetchContent without cache violates CI timing AC**: JUCE repository is ~1.5GB. Without caching, first-run CI download takes 15–25 minutes on GitHub-hosted runners, making AC-2 ("within 20 minutes") unachievable on the initial run that matters most.

**G7 [Strongly Recommended] `-Werror` scoped to all includes including JUCE**: JUCE headers generate warnings on GCC 13+, MSVC 2022, and clang 17 in certain configurations. Global `-Werror` will break CI whenever JUCE updates. The project has no control over JUCE's warning posture.

**G8 [Strongly Recommended] Windows toolchain implicit**: `windows-latest` defaults to MSVC (VS2022) but this is undocumented. A future GitHub runner update could change behavior silently.

### 01-03 (Project model + audio thread)

**G9 [Must-Have] No schema version in .layerz files**: PROJECT.md explicitly mentions a migration tool for v1→v2+. Without a `schema_version` field in every saved file, that tool is blind. Files written in F0 will be indistinguishable from files written in any future schema. This cannot be added retroactively to files already in the wild.

**G10 [Must-Have] Float `operator==` fragile after JSON round-trip**: `velocity`, `swing_global`, `drag`, `push` are floats. After JSON serialization (`0.1f` → `"0.1"`) and deserialization, bit-exact equality is non-deterministic across platforms and JSON libraries. The Catch2 round-trip test will produce false negatives.

**G11 [Must-Have] `loadFromFile()` error handling unspecified**: No mention of behavior on malformed JSON, truncated file, unknown schema version, or missing fields. Corrupt `.layerz` files will occur (crash during save, disk error, user editing). Unspecified error handling defaults to crash or UB.

**G12 [Must-Have] `Clock::process()` return type contradicts no-allocation rule**: "Returns list of beat events" implies `std::vector<BeatEvent>` — a heap allocation on every audio callback. The plan simultaneously says "no allocations inside process()". Contradiction; the return type must be a non-owning view over a pre-allocated buffer.

**G13 [Strongly Recommended] Atomic file write not specified**: `saveToFile()` with direct overwrite will produce a corrupt `.layerz` file if the process is killed or loses power mid-write. Write-to-temp + rename is atomic on Linux/macOS/Windows (POSIX rename guarantee; Windows MoveFileEx with MOVEFILE_REPLACE_EXISTING).

**G14 [Strongly Recommended] Unknown JSON fields: behavior unspecified**: When a newer `.layerz` is opened in an older build (or a user hand-edits the file), unknown fields will cause nlohmann/json to throw by default. Forward compat requires explicit `ignore_unknown_fields` behavior.

**G15 [Strongly Recommended] Rectangular sine burst produces click artifact**: A 1ms rectangular-windowed sine burst has a step discontinuity at attack and release that produces a broadband click. This click would be mistaken for a timing error when verifying AC-2/AC-3 alignment.

---

## 4. Upgrades Applied to Plans

### Must-Have (Applied)

| # | Finding | Plan | Section Modified | Change Applied |
|---|---------|------|-----------------|----------------|
| 1 | G1: Benchmark single-run invalidity | 01-01 | Task 1 action | Changed to 5 independent 10-second windows per config; aggregates mean+stddev |
| 2 | G2: `-O2` not Release | 01-01 | Task 1 build comment | Changed to `-O3 -DNDEBUG` |
| 3 | G5: JUCE splash screen missing | 01-02 | Task 1 action | Added `target_compile_definitions(...JUCE_DISPLAY_SPLASH_SCREEN=1)` |
| 4 | G6: FetchContent cache absent | 01-02 | Task 3 CI YAML | Added `actions/cache@v4` step keyed on CMakeLists.txt hash |
| 5 | G9: No schema_version | 01-03 | Task 1 Schema.h | Added `CURRENT_SCHEMA_VERSION = 1` constant; `schema_version` as first Project field |
| 6 | G10: Float operator== fragile | 01-03 | Task 2 action | Specified epsilon comparison `< 1e-6f` for all float fields in operator== |
| 7 | G11: loadFromFile() error handling | 01-03 | Task 2 action | Specified `juce::Result` return; malformed JSON, truncated file, unknown version all return error, never UB |
| 8 | G12: Clock::process() return type | 01-03 | Task 3 action | Changed to `juce::ArrayRef<BeatEvent>` (view over pre-allocated internal array); added `BeatEvent` struct definition |

### Strongly Recommended (Applied)

| # | Finding | Plan | Section Modified | Change Applied |
|---|---------|------|-----------------|----------------|
| 1 | G3: No stddev in TSV | 01-01 | Task 1 action | Added `stddev_cpu` column to TSV spec |
| 2 | G4: No weakest-machine policy | 01-01 | Human-action checkpoint | Added step 6: weakest machine at buffer=256 sets plugin cap; stddev > 5% = re-run |
| 3 | G7: -Werror hits JUCE headers | 01-02 | Task 1 action | Added `target_include_directories(...SYSTEM...)` requirement for JUCE headers |
| 4 | G13: Non-atomic file write | 01-03 | Task 2 action | Added write-to-`.layerz.tmp` + `juce::File::moveFileTo()` requirement |
| 5 | G14: Unknown JSON fields unspecified | 01-03 | Task 2 action | Specified unknown fields must be silently skipped |
| 6 | G15: Rectangular window click | 01-03 | Task 3 action | Added Hann window with formula; explained why rectangular produces artifact |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | ASan/UBSan CI jobs | F0 is pre-production stubs; no allocations/UB paths yet. Add in F1 when real logic exists. |
| 2 | macOS code signing for AU in CI | CI builds the AU artifact; loading/testing it requires signing. Deferred to F7 distribution plan. |
| 3 | Windows CI explicit MSVC version pin | `windows-latest` is stable on GitHub for MSVC 2022. Low risk; pin in F7 if flakiness observed. |
| 4 | Schema migration tooling | The `schema_version` field added here is the prerequisite. Migration tool is a F7 task per ROADMAP. |
| 5 | VoiceBench integration into CMake | Spike tool intentionally standalone. Wire into CMake only if regression bench is needed post-F0. |

---

## 5. Audit & Compliance Readiness

**Evidence quality**: The spike documents (F0.99, F0.99b) produce machine-readable measurements and written decisions committed to `docs/spikes/`. This is sufficient audit trail for why architectural caps were chosen.

**Silent failure prevention**: After applied fixes, `loadFromFile()` must not silently succeed on corrupt data. The error return + explicit unknown-field skip gives observable failure modes.

**Post-incident reconstruction**: `.layerz` files are UTF-8 JSON (human-readable + diffable). Schema version is present from day one. Sufficient for post-incident replay.

**Ownership**: Solo developer. No multi-party approval chain exists. The PAUL plan/audit/apply/unify loop serves as the formal review record.

**Gap**: No audio-thread allocation checker in debug builds is specified. This is the highest-latent-risk gap remaining after applied fixes. The PROJECT.md mentions "audio-thread checks in debug builds" (F1-AC4 requires it). The F0 plans do not wire it. This is acceptable for F0 (no real audio code yet) but must be addressed in the F1 plan.

---

## 6. Final Release Bar

**What must be true before these plans proceed to APPLY:**
- All must-have and strongly-recommended findings applied ✓ (done above)
- Human commits to running the bench on actual hardware (not emulated/virtualized)
- The F0.99b thread-safety decision is made before 01-03 APPLY begins

**Risks remaining if shipped as-is (post-fixes):**
- Windows CI MSVC version is implicit — acceptable risk for now
- No audio-thread allocation checker yet — acceptable for F0 stub, must land in F1
- Schema_version is hardcoded; no migration logic — acceptable, migration is F7 scope

**Sign-off**: I would approve these plans for APPLY. The core architecture is sound, the dependency sequencing is correct, and the applied fixes close all release-blocking gaps.

---

**Summary:** Applied 8 must-have + 6 strongly-recommended upgrades. Deferred 5 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
*Audited: 2026-05-28*
