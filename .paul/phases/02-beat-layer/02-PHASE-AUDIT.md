# Enterprise Plan Audit Report

**Plans:** 02-01-PLAN.md, 02-02-PLAN.md
**Audited:** 2026-05-29
**Verdict:** Conditionally acceptable — applied fixes make plans enterprise-ready

---

## 1. Executive Verdict

Conditionally acceptable. Both plans are well-structured with clear scope, correct
dependencies, and tight boundaries. However, eight must-have gaps would have caused
real failures at runtime: one would break the core safety guarantee of the entire audio
architecture (AC4 mutex violation), two would cause memory corruption or crashes, and
three would create an infinite GUI mutation loop. All applied. Plans are now approvable.

---

## 2. What Is Solid

- **Scope split is correct.** Engine (02-01) and UI (02-02) separated cleanly. Dependencies
  are genuine: 02-02 needs VoiceBank and BeatSequencer to exist before wiring controls.
- **Boundary sections are specific.** "No BASS layer strip", "No Faust", "No groove controls"
  prevent scope creep with explicit, named constraints.
- **Faust deferral is well-reasoned.** Using JUCE DSP for F1 drums removes a toolchain
  dependency without compromising the architecture. Faust enters at F4/F5 where it genuinely
  matters (granular synthesis complexity).
- **Hardcoded test pattern strategy is sound.** Seeding via postMutation in constructor means
  the test pattern goes through the exact same mutation path as GUI-authored patterns. This
  tests the real pipeline before GUI exists.
- **BeatSequencer read-from-snapshot design is correct.** Single snapshot at top of processBlock,
  passed by reference to sequencer — no repeated snapshot() calls inside the audio loop.
- **Optimistic postMutation pattern (02-02).** Checking value-changed before calling postMutation
  avoids spam mutations from slider drag noise.

---

## 3. Enterprise Gaps Identified

### 02-01 Gaps

**G1 [Must-Have] ProjectStore::snapshot() acquires mutex — violates F1-AC4**
The F0.99b spike explicitly committed to "upgrade to C++20 `atomic<shared_ptr>` in F1 to
eliminate the mutex entirely." The plan's Task 3 states `auto snap = store_.snapshot(); — short mutex`
and does NOT include the upgrade. ACCEPTANCE.md §F1-AC4 says "no lock acquisition on audio thread."
The current mutex IS a user-level lock. AC4 cannot be satisfied without this upgrade.

**G2 [Must-Have] Buffer overflow in DrumVoice::process()**
process(buf, startSample, numSamples) — if a trigger fires at sampleOffset=252 in a 256-sample block,
the voice's full 441-sample (10ms) envelope extends 437 samples beyond the buffer boundary.
No clamping specified. Writing past buf.getNumSamples() is undefined behaviour and likely
a crash on subsequent blocks.

**G3 [Must-Have] ProjectStore.cpp missing from files_modified**
Task 1 says "Update ProjectStore.cpp serialisation/deserialisation" but the file is not in
the frontmatter. Conflict detection would miss it; CI diff tracking is wrong.

**G4 [Must-Have] BeatSequencer accesses patterns[0].layers[0] without bounds guards**
patterns.empty() is guarded but patterns[0].layers.empty() is not. If setStateInformation()
loads a .layerz with no layers (valid JSON, zero layers), BeatSequencer crashes the audio thread.
Silent failure in production — no assertion, just invalid memory access.

**G5 [Must-Have] Duplicate step events cause double-trigger**
BeatSequencer linear scan finds FIRST matching event and triggers. If two events share the
same step (reachable via malformed .layerz import), voice triggers twice in one block at the
same sample offset. Volume doubles silently; no warning surfaced.

### 02-02 Gaps

**G6 [Must-Have] Wrong notification API — infinite postMutation loop**
updateFromSnapshot() specifies "use setValueNotifyingHost=false or ScopedValueSetter."
Both are wrong for juce::Slider:
- `setValueNotifyingHost(false)` is only valid for AudioProcessorParameter
- `ScopedValueSetter` sets a value for a duration but does not suppress notifications

Correct API: `slider.setValue(value, juce::dontSendNotification)`. Using the wrong API means:
timer fires → updateFromSnapshot → slider.setValue (notifying) → onChange fires →
postMutation → next frame → updateFromSnapshot → infinite loop of mutations per 33ms tick.
Result: rapid Project corruption and likely crash.

**G7 [Must-Have] StepRow::updateFromSnapshot button state update not specified**
StepRow updates 16 TextButtons in updateFromSnapshot but does not specify notification suppression
for buttons. `setToggleState(state)` without `juce::dontSendNotification` fires onClick, which
triggers postMutation — same infinite loop as G6 but for 16 buttons simultaneously.

**G8 [Must-Have] Event erase removes only first occurrence**
StepRow onClick erase: "erase from events where event.step==i" — standard erase() removes one
element. If duplicates exist, only the first is removed, leaving phantom events that continue
triggering. Must use std::remove_if + erase idiom to remove ALL matching steps.

### Strongly Recommended

**SR1** DrumVoice retrigger behavior undefined — should reset envelope on retriger (correct drum behavior).
**SR2** LCG noise always starts at same seed — snare/hat identical sequence each launch.
**SR3** F0 round-trip test may break after Schema.h changes — must explicitly verify.
**SR4** Optimistic UI update for step buttons — 33ms lag before visual feedback without it.
**SR5** `proc_.clock_` accessed directly instead of via `getClock()` accessor.

---

## 4. Upgrades Applied to Plans

### Must-Have (Applied)

| # | Finding | Plan | Section Modified | Change Applied |
|---|---------|------|-----------------|----------------|
| 1 | G1: snapshot() mutex violates AC4 | 02-01 | Task 3 action | C++20 std bump; ProjectStore upgrade to atomic<shared_ptr> with full code pattern |
| 2 | G2: DrumVoice buffer overflow | 02-01 | Task 2 action | Bounds clamping: jmin(remaining, avail); stop writing at buffer boundary |
| 3 | G3: ProjectStore.cpp missing | 02-01 | Frontmatter + Task 1 files | Added ProjectStore.h/.cpp to files_modified and Task 1 files list |
| 4 | G4: BeatSequencer bounds gaps | 02-01 | Task 3 action | 5-guard chain: patterns/layers/BEAT-type/drum_tracks.empty() before access |
| 5 | G5: Duplicate step double-trigger | 02-01 | Task 3 action | break after first match; prevents silent volume doubling |
| 6 | G6: Wrong slider notification API | 02-02 | Task 1 action | Replaced with juce::dontSendNotification; explained infinite loop risk |
| 7 | G7: Button notification not suppressed | 02-02 | Task 1 action | setToggleState with juce::dontSendNotification in updateFromSnapshot |
| 8 | G8: Event erase incomplete | 02-02 | Task 1 action | std::remove_if + erase idiom; removes ALL events with matching step |

### Strongly Recommended (Applied)

| # | Finding | Plan | Section Modified | Change Applied |
|---|---------|------|-----------------|----------------|
| 1 | SR1: Retrigger behavior | 02-01 | Task 2 action | Explicit: trigger() resets envelope from start regardless of active state |
| 2 | SR2: LCG seed | 02-01 | Task 2 action | std::random_device{}() seed in prepare() |
| 3 | SR3: F0 round-trip | 02-01 | Verification | Added explicit check: F0-era Schema test still passes after DrumTrack addition |
| 4 | SR4: Optimistic update | 02-02 | Task 1 action | onClick updates button visual immediately; timer confirms on next tick |
| 5 | SR5: Clock accessor | 02-02 | Task 2 action | proc_.getClock().isStandaloneMode() instead of direct member access |

### Deferred (Can Safely Defer)

| # | Finding | Rationale |
|---|---------|-----------|
| 1 | Voice polyphony per type | Only one voice per type; retriger is correct for drums. Polyphony needed when BASS layer adds overlapping mono notes (F2). |
| 2 | Play head cursor on steps | Visual beat-position indicator on StepRow. Nice-to-have; not in AC criteria. |
| 3 | Keyboard shortcuts | Not in F1 scope; no AC requires it. |
| 4 | MIDI input to BEAT layer | Explicitly F2 scope (F2-AC5 is the BASS MIDI criterion). |

---

## 5. Audit & Compliance Readiness

**Evidence:** AudioThreadGuard + LAYERZ_ASSERT_NOT_AUDIO_THREAD gives observable debug evidence of AC4 violations. Post-applied, snapshot() leaves no user-level lock on audio thread.

**Silent failure prevention:** Bounds guards on BeatSequencer prevent silent crashes. Duplicate-step break prevents silent volume doubling. dontSendNotification prevents silent mutation loops.

**Post-incident reconstruction:** ProjectStore JSON is the canonical state; all mutations go through postMutation. Post-mortem: any audio artifact can be traced to a specific Project state via the last saved snapshot.

**Ownership:** Solo developer. PAUL APPLY/UNIFY loop is the formal review record.

---

## 6. Final Release Bar

**Must be true before APPLY:**
- C++20 upgrade applied and build confirmed on macOS
- snapshot() confirmed lock-free by code inspection
- All buffer bounds clamped in DrumVoice::process()
- BeatSequencer guards present for all array accesses

**Remaining risks post-fixes:**
- ProfileConfig caps are PRELIMINARY (synthetic bench). F1 adds real drum voices — recalibration obligation from F0 is now due.
- Hat/snare/perc voices are single-instance (no polyphony). Not a bug for drums; documented design.

**Sign-off:** Plans are approvable for APPLY with applied fixes in place.

---

**Summary:** Applied 8 must-have + 5 strongly-recommended upgrades. Deferred 4 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
*Audited: 2026-05-29*
