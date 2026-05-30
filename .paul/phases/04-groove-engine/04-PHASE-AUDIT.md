# Enterprise Plan Audit Report

**Plans:** 04-01, 04-02, 04-03
**Audited:** 2026-05-30
**Verdict:** Conditionally acceptable — applied fixes make plans enterprise-ready

---

## 1. Executive Verdict

Conditionally acceptable. Plans are architecturally well-structured with correct sequencing
(spike → engine → UI), clear scope limits, and solid RT safety discipline. Four must-have
gaps found: one directional inversion that would silently produce wrong PUSH behavior, one
missing Clock accessor blocking a required computation, one ordering bug that would corrupt
FRACTURE determinism across bar boundaries, and one phase-reset issue causing click artifacts.
All applied. Plans are now approvable.

---

## 2. What Is Solid

- **Spike-first sequencing** (04-01 → 04-02 → 04-03) is correct: implementation strategy
  decided before code is written, preventing architectural backtrack.
- **carry-forward queue strategy** (Strategy B) is the right choice. Strategy A fails at the
  acceptance criteria requirements (30ms > 1 block). Strategy C adds complexity not needed
  at F3. The reasoning is sound.
- **FRACTURE LCG with fixed seed** produces deterministic sequences — correct for F3-AC5.
  The seed-reset-at-loop-start approach is the right design for a groovebox where users
  expect reproducibility within a session.
- **Template + morph as Schema fields** (template_name + morph_amount) is the correct
  architecture: state is serialisable, morph is continuous, templates are static (no CRUD).
- **MonoSynth ADSR deferred from F2** correctly placed here: STUTTER requires gated
  envelope shape, so F3 is the right time to add it.
- **AestheticResolver produces pre-allocated output** (no heap allocation in audio path) ✓.

---

## 3. Enterprise Gaps Identified

**G1 [Must-Have] PUSH boundary description inverted in spike**
04-01 spike says "PUSH: if displaced_offset ≥ blockSize" — but PUSH SUBTRACTS from offset
(formula: offset = beatSampleOffset + drag_samples - **push_samples**). PUSH makes offsets
SMALLER, so the boundary case is displaced < 0, not ≥ blockSize. The implementation in
04-02 Task 2 has the correct formula but the spike documentation is wrong. If an implementer
follows the spike literally instead of the formula, PUSH produces wrong behavior silently.

**G2 [Must-Have] Clock does not expose samplesPerBeat() accessor**
04-02 says "PluginProcessor passes `clock_.samplesPerBeat()`" but samplesPerBeat_ is a
private field on Clock with no public accessor. This creates a compile error. The accessor
must be added to Clock.h.

**G3 [Must-Have] FRACTURE seed reset before carry-forward drain corrupts determinism**
04-02 says resetFractureSeed() is called "at pattern loop start (beat_index % 16 == 0)".
But drainPending() also runs at the start of each block. If a deferred event from bar N
is in the queue and the seed is reset before draining, the LCG state for that event is
DIFFERENT from what was computed when it was queued. The FRACTURE check already happened
for that event (in the previous block), so this specific event is fine — but any events
generated FROM the drain with the new seed would be wrong. Must drain THEN reset.

**G4 [Must-Have] MonoSynth.trigger() resets phase_ → click artifact on STUTTER**
STUTTER emits multiple trigger() calls per step. Each trigger() resets phase_=0.0,
creating a sawtooth discontinuity (the wave jumps from its current position to phase=0).
At 220-460Hz, this is an audible click at each stutter hit boundary. Must add a
`triggerRetain(velocity)` path that resets envelope but preserves phase_.

**G5 [Strongly Recommended] AestheticResolver kQueueCap=8 insufficient for ROLL × STUTTER**
ROLL ×8 + STUTTER reps=8 applied simultaneously = 64 sub-events per input event.
With kQueueCap=8, the queue overflows silently (maxOut exceeded, events dropped).
kQueueCap must be ≥ 8×8 = 64.

**G6 [Strongly Recommended] findTemplate() with empty name crashes applyMorph()**
applyMorph(neutral, *findTemplate(name, type)) — if name is empty and morph_amount > 0,
findTemplate returns nullptr and dereferencing it is UB. Must guard null return.

---

## 4. Upgrades Applied

### Must-Have (Applied)

| # | Finding | Plan | Section | Change Applied |
|---|---------|------|---------|----------------|
| 1 | G1: PUSH boundary inverted | 04-01 | Task 1 Strategy B | Corrected: PUSH case is displaced < 0 (not ≥ blockSize); formula explanation added |
| 2 | G2: Clock::samplesPerBeat() missing | 04-02 | Task 2 BeatSequencer | Added explicit requirement to add Clock.h accessor |
| 3 | G3: Seed reset before drain corrupts FRACTURE | 04-02 | Task 2 BeatSequencer | Ordering stated explicitly: drainPending() THEN resetFractureSeed() |
| 4 | G4: MonoSynth phase reset causes click | 04-02 | Task 2 STUTTER section | Added triggerRetain() spec; STUTTER sub-events 2..N use triggerRetain |

### Strongly Recommended (Applied)

| # | Finding | Plan | Section | Change Applied |
|---|---------|------|---------|----------------|
| 1 | G5: kQueueCap=8 insufficient | 04-03 | Task 1 action | Added note to increase kQueueCap to 64 in 04-02 |
| 2 | G6: null findTemplate with empty name | 04-03 | Task 1 action | findTemplate() null-guard + applyMorph() call guard |

### Deferred

| # | Finding | Rationale |
|---|---------|-----------|
| 1 | ROLL+STUTTER simultaneous edge case behavior | Single aesthetic active at once is the common case; simultaneous application documented as possible but untested |
| 2 | FRACTURE probability distribution uniformity | LCG modulo bias at 50% is < 0.1% error; acceptable for groove use |

---

## 5. Audit & Compliance Readiness

- **Evidence**: Spike document (F3.99) provides decision trail. LCG seed committed to .layerz files enables post-incident replay.
- **Silent failure prevention**: G1 (wrong PUSH), G3 (FRACTURE corruption), G4 (click) were all silent failures — user would hear wrong behavior with no error. All addressed.
- **Determinism**: FRACTURE seed + drain ordering (G3 fix) ensures reproducible sequences — critical for the author's validation gate (F3-AC7).

---

## 6. Final Release Bar

Must be true before APPLY:
- Clock::samplesPerBeat() accessor added
- PUSH boundary: displaced < 0 case handled (not ≥ blockSize)
- drainPending() before resetFractureSeed() order enforced
- MonoSynth::triggerRetain() added for STUTTER continuity
- kQueueCap ≥ 64

**Sign-off**: Plans are approvable with applied fixes.

---

**Summary:** Applied 4 must-have + 2 strongly-recommended. Deferred 2.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow — 2026-05-30*
