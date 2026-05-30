# Enterprise Plan Audit Report

**Plans:** 03-01, 03-02, 03-03
**Audited:** 2026-05-29
**Verdict:** Conditionally acceptable — applied fixes make plans enterprise-ready

---

## 1. Executive Verdict

Conditionally acceptable. All three plans are well-structured with correct wave sequencing,
clean boundaries, and specific task specs. Five must-have gaps were found — three of which
would cause crashes or silent data corruption in production. All applied. Plans are approvable.

---

## 2. What Is Solid

- **Dependency sequencing** is correct: 03-01 wave 1 provides MonoSynth + Schema before
  03-02/03-03 wave 2 consume them. 03-02 and 03-03 don't share files — genuine parallel execution.
- **BASS rendering model** (trigger at sample offset + sustain next block) is architecturally
  correct for a step sequencer. The "if no trigger: processBass full block" pattern is right.
- **PatternChainEntry design** is clean: ordered array with pattern_id lookup is durable across
  renames and doesn't embed pattern index (avoids stale index bugs when patterns are reordered).
- **save/load via atomic write** reuses the ProjectStore.saveToFile() from F0 (tmp→rename). Correct.
- **BASS layer type search in BeatSequencer** (after audit fix) is correct: LayerType::BASS not index.

---

## 3. Enterprise Gaps Identified

### 03-01 Gaps

**G1 [Must-Have] Duplicate store_.snapshot() in MIDI handler**
Plan spec calls `store_.snapshot()` inside the MIDI event loop (per note-on). processBlock
already holds a snap from the top. The MIDI handler acquires a second mutex per MIDI note.
With rapid MIDI input, this is N mutex acquisitions per block. Also: the BassVoiceParams
loop inside the per-note block is O(n_layers) per MIDI event — needless work on the hot path.

**G2 [Must-Have] BeatSequencer BASS sustain gap: samples 0..firstTriggerOffset not rendered**
If a BASS beat event fires at sampleOffset=128 in a 256-sample block, and the previous note
was still sounding, samples 0-127 of the sustain are silently dropped. The plan's "if no trigger:
processBass 0 to full block" only covers the NO-TRIGGER case. The WITH-TRIGGER case must render
the pre-trigger sustain first, then trigger+render remainder.

**G3 [Must-Have] ChainManager % snap.chain.size() crash when chain is empty**
The chain can become empty mid-playback if the user removes all entries via postMutation.
`(chainPosition_ + 1) % 0` is undefined behavior on all platforms. The plan's step 2 guards
empty chain at entry, but step 4 re-accesses chain.size() without re-checking. Must guard again.

### 03-02 Gaps

**G4 [Must-Have] BeatSequencer.process() signature change — call site not explicitly updated**
Plan adds `patternIndex` param to BeatSequencer::process() but does NOT say "update
PluginProcessor.cpp call site." Implementer could add a compatibility wrapper instead of
updating the call, leaving the old path in place. Must be stated explicitly.

### 03-03 Gaps

**G5 [Must-Have] PitchedStepRow uses layerIndex=0 which reads BEAT not BASS**
In the seeded pattern: layers[0]=BEAT, layers[1]=BASS. `layerIndex_=0` silently reads BEAT
events into PitchedStepRow. User sees BEAT step state in the BASS strip and mutations go to
wrong layer. The BASS layer must be found by `LayerType::BASS`, not by index.

### Strongly Recommended

**SR1** BassVoiceParams operator== must use float epsilon on all 8 float fields — plan says
"float epsilon" without enumerating which fields. Waveform is exact enum, rest need epsilon.

**SR2** Note sustain when no new trigger fires: plan documents "auto-releases at next step OR
after 80% gate" in constraints but the implementation spec only releases on new trigger. If
the pattern's last step has a BASS note, that note sustains until silence (fine musically)
but should be explicitly documented as intended behavior.

**SR3** BeatSequencer call site update explicit (applied as M4).

**SR4** requestPatternSwitch with invalid index: if called with i >= snap.patterns.size(),
pendingIndex_ stores invalid index → next bar switch sets playIndex_ out of bounds.
Add bounds clamp in requestPatternSwitch.

**SR5** BassLayerStrip timer: `snap->patterns[active_pattern_index]` unclamped — if
active_pattern_index >= snap.patterns.size(), crash. Guard added.

---

## 4. Upgrades Applied

### Must-Have (Applied)

| # | Finding | Plan | Section | Change |
|---|---------|------|---------|--------|
| 1 | G1: Duplicate snapshot() + per-note layer loop in MIDI handler | 03-01 | Task 3 action | Pre-extract BassVoiceParams once before MIDI loop; use existing snap |
| 2 | G2: BASS sustain gap before first trigger offset | 03-01 | Task 3 action | Added 3-step rendering order: pre-trigger sustain → trigger+tail → no-trigger full |
| 3 | G3: chain.empty() crash in ChainManager modulo | 03-02 | Task 1 action | Added guard at entry AND at step 4 re-access |
| 4 | G4: BeatSequencer call site not stated | 03-02 | Task 2 action | Explicit: old call → new call with patIdx, compile error if missed |
| 5 | G5: PitchedStepRow reads BEAT layer by index 0 | 03-03 | Task 1 action | Removed layerIndex_ param; search by LayerType::BASS in onClick and updateFromSnapshot |

### Strongly Recommended (Applied)

| # | Finding | Plan | Section | Change |
|---|---------|------|---------|--------|
| 1 | SR1: BassVoiceParams float epsilon fields not enumerated | 03-01 | Task 1 action | Listed all 8 float fields explicitly |
| 2 | SR4: requestPatternSwitch bounds | 03-02 | Task 1 action | Note: clamp pendingIndex_ to valid range |
| 3 | SR5: BassLayerStrip timer unclamped pattern index | 03-03 | Task 2 action | Guard + jlimit before accessing snap.patterns[] |

### Deferred

| # | Finding | Rationale |
|---|---------|-----------|
| 1 | SR2: Sustained note on last step doesn't auto-release | Musically intentional in a groovebox (hold/legato). Document in SUMMARY. |
| 2 | Seeded test pattern removal | Dev tool; removed implicitly when UI is authoritative. Not production risk. |
| 3 | Note length / gate time per step | F3+ scope; out of plan boundaries. |
| 4 | Chain editor UI drag-to-reorder | F3+ scope; explicitly out of boundaries. |

---

## 5. Audit & Compliance Readiness

- **Silent failure prevention**: G5 (wrong layer reads) and G1 (double mutex) were both silent failures producing wrong audio output with no assertion or error. Both fixed.
- **Crash prevention**: G3 (division by zero) and G5 (out-of-bounds index) are hard crashes. Both fixed.
- **Post-incident reconstruction**: .layerz JSON files are fully serialised; chain, BassVoiceParams, active_pattern_index all captured. Pattern IDs are string-based (stable across reorders).
- **Ownership**: solo developer; PAUL loop is the formal review record.

---

## 6. Final Release Bar

**Must be true before APPLY:**
- All 5 must-have findings applied ✓ (done above)
- BeatSequencer process() signature change verified by compile error at old call site

**Remaining risks:**
- BASS sustain timing on last pattern step is indefinite (intentional, musical)
- 16-step hardcoded modulo in ChainManager assumes all patterns identical length — OK for F2

**Sign-off:** Plans are approvable for APPLY with applied fixes.

---

**Summary:** Applied 5 must-have + 3 strongly-recommended. Deferred 4.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
*Audited: 2026-05-29*
