# LAYERZ — ACCEPTANCE.md

Acceptance criteria are written in Given/When/Then form, scoped per phase, each with a clear measurable outcome. The canonical product (`Project` model) and the mechanical modes (`PROFILE_PLUGIN` vs `PROFILE_STANDALONE`) each have dedicated criteria proving the distinction is real, not cosmetic. Spike acceptance is open-ended — a spike's output is a written decision in `docs/spikes/`, not a Given/When/Then.

## §F0 — Scaffolding

**F0-AC1 — Cross-platform build.**
Given the LAYERZ source on a clean machine running Linux, macOS (Apple Silicon), or Windows, with CMake and a recent JUCE checkout available,
When the build is invoked with `cmake --build`,
Then VST3, AU (macOS only), LV2, and standalone targets all produce loadable artifacts without errors or warnings categorized as errors.

**F0-AC2 — Plugin loads in two hosts without crash.**
Given the VST3/AU build,
When LAYERZ is loaded in Reaper (Linux/macOS/Windows) and in Bitwig (Linux/macOS/Windows) on a fresh project,
Then the plugin opens, exposes a non-empty GUI, and shuts down cleanly when the project is closed.

**F0-AC3 — Click sync with host transport.**
Given LAYERZ loaded as plugin in a host with transport set to 120 BPM,
When the host starts transport,
Then the click sounds on every quarter beat aligned to the host's PPQ, and remains aligned for at least 8 bars without measurable drift (≤ 1 sample/bar).

**F0-AC4 — Click runs on internal clock when standalone.**
Given LAYERZ in standalone mode with internal BPM set to 120,
When the user presses play,
Then the click sounds on every quarter beat using the internal clock, with no dependency on any external transport.

**F0-AC5 — Profile mechanical difference is verifiable.**
Given LAYERZ with a stress-test pattern requiring N voices (where N exceeds the plugin profile cap and is below the standalone profile cap),
When the pattern is loaded under `PROFILE_PLUGIN`, then again under `PROFILE_STANDALONE`,
Then under `PROFILE_PLUGIN` the engine drops voices according to the documented voice-stealing policy and the CPU stays under target, and under `PROFILE_STANDALONE` all N voices play. The difference is measurable and reproducible.

**F0-AC6 — Project model round-trip.**
Given a hand-authored `.layerz` JSON file representing a small project (one pattern, two layers, a few events),
When LAYERZ loads it and immediately saves it back,
Then the resulting file is semantically identical to the input (deep equality on the model, ignoring formatting whitespace).

## §F1 — BEAT layer

**F1-AC1 — A BEAT pattern plays.**
Given a project with one BEAT layer, four drum voices assigned (kick, snare, hat, perc), and a 16-step pattern with events on steps 1, 5, 9, 13 (kick) and 5, 13 (snare),
When the user presses play,
Then audible kick hits land on those steps and snare hits land on steps 5 and 13, looping until stopped.

**F1-AC2 — Voice parameter changes are audible.**
Given a kick voice playing on every quarter,
When the user adjusts the voice's pitch envelope decay from 50ms to 500ms,
Then the audible kick clearly changes character (longer pitch sweep), and the parameter change is reflected in the saved `Project`.

**F1-AC3 — Mixer mute/solo works.**
Given a pattern with all four drum voices active,
When the user mutes the hat voice,
Then the hat stops sounding immediately at the next event, and the project still plays the other three voices uninterrupted.

**F1-AC4 — Real-time safety on audio thread.**
Given a debug build with audio-thread allocation/lock checks enabled,
When LAYERZ plays a BEAT pattern for 60 seconds with the user toggling steps, adjusting voice parameters, and muting/unmuting,
Then no allocation, lock acquisition, or file I/O is recorded on the audio thread.

## §F2 — BASS + multi-pattern + chain

**F2-AC1 — BASS layer plays pitched notes.**
Given a project with a BASS layer using the mono-synth voice, a pattern with events at steps 1, 4, 7, 11 with pitches C2, E2, G2, B♭2 respectively,
When the user presses play,
Then the audible notes sound at the correct pitches in the correct order, mono-synth behavior is correct (no overlap, glide if enabled).

**F2-AC2 — Multiple patterns coexist.**
Given a project containing two patterns A and B with different BEAT and BASS events,
When the user switches the active pattern from A to B,
Then on the next loop boundary, pattern B's events sound and pattern A's events stop, without audio glitches.

**F2-AC3 — Chain plays patterns in order.**
Given a chain configured as A → B → A → B with 1 bar per pattern,
When the user starts playback,
Then the audible sequence is A, B, A, B, looping the chain end to start without timing jumps.

**F2-AC4 — Save and reload preserves state.**
Given a project with two layers, two patterns, a chain, and modified voice parameters,
When the user saves to `test.layerz`, closes the project, and reopens `test.layerz`,
Then all layers, patterns, chain entries, voice parameters, and the active scene are restored to the state at save time.

**F2-AC5 — Plugin receives MIDI from host for BASS.**
Given LAYERZ loaded as plugin with the BASS layer set to "receive MIDI from host",
When the host sends a MIDI note-on event,
Then the BASS voice triggers at the corresponding pitch with the host's velocity, within one audio block of the MIDI timestamp.

## §F3 — Groove engine

**F3-AC1 — DRAG produces measurable negative offset.**
Given a BEAT pattern with DRAG amount = 0 on all steps,
When the user sets DRAG amount to 50% on the snare lane,
Then the snare hits arrive measurably late relative to the grid (mean offset between -10 and -30ms at 120 BPM, depending on configured curve), and at DRAG = 0 the offset is 0 ± 1 sample.

**F3-AC2 — PUSH produces measurable positive offset.**
Given the same pattern,
When the user sets PUSH = 50% on the snare,
Then the snare arrives measurably early (mean offset between +5 and +25ms, mirror of DRAG within tolerance).

**F3-AC3 — ROLL multiplies step density.**
Given a BEAT pattern with one snare hit at step 13,
When the user applies ROLL ×4 to that step,
Then four audible snare hits sound between steps 13 and 14, evenly spaced, with the configured velocity envelope.

**F3-AC4 — STUTTER repeats with gating.**
Given a BASS event at step 1 with STUTTER enabled (length 1/16, repetitions 4),
When the pattern plays,
Then four short gated repetitions of the bass note sound starting at step 1, each at 1/16 of the step length.

**F3-AC5 — FRACTURE probabilistic firing.**
Given a BEAT pattern with the hat lane set to FRACTURE probability 50% on every 16th step, with a fixed seed,
When the pattern is played twice from the start with the same seed,
Then the sequence of hat events is identical between the two playthroughs, and over 100 steps the firing rate is approximately 50% (between 40 and 60 fires).

**F3-AC6 — Template + morph is non-cosmetic.**
Given a layer with template "Lurch Soul" applied (DRAG strong, ROLL light, FRACTURE off),
When the user advances the morph knob from 0 to 100%,
Then the audible character changes continuously and measurably (DRAG amount, ROLL probability, and FRACTURE probability all shift along their template-defined morph curves), and the resulting `Layer.aesthetics` block in the model differs at each morph value.

**F3-AC7 — The validation recording is possible.**
Given LAYERZ at the end of F3,
When the author records a 30-second beat with the intent of "audible lurch character",
Then the author confirms the feel is audibly correct without editing in another tool. (Subjective gate; satisfied by the author's signed-off recording in `docs/validation/`.)

## §F4 — HARMONIC + MELODIC

**F4-AC1 — Additive voice produces controlled spectrum.**
Given a HARMONIC layer with the additive voice configured with 8 partials of decreasing amplitude,
When a note plays,
Then a spectrogram of the output shows exactly the configured partials at the expected frequencies and amplitudes, within reasonable analysis tolerance.

**F4-AC2 — BLOOM produces a granular cloud.**
Given a HARMONIC layer with BLOOM voice, source buffer loaded, grain size 100ms, density 20 grains/sec, position drift enabled,
When a note triggers,
Then the audible result is a granular cloud (continuous, textured, with audible grain overlap), distinct from the additive voice's clean tonal output.

**F4-AC3 — Scale constraint snaps notes.**
Given a MELODIC layer with scale constraint set to "C minor pentatonic",
When the user enters a note at pitch B3 (not in scale),
Then the resulting event's pitch is snapped to the nearest in-scale note (A3 or C4 per the snap rule), and the snap is reflected in the saved `Project`.

**F4-AC4 — Motif mutation operators work.**
Given a MELODIC pattern with notes [C4, E4, G4, B4],
When the user applies "invert" then "retrograde",
Then the resulting pattern is the algorithmically-correct invert-then-retrograde of the input (verifiable against an independent calculation).

**F4-AC5 — Full four-layer pattern plays.**
Given a project with one event in each of BEAT, BASS, HARMONIC, MELODIC layers, on the same step,
When the pattern plays,
Then all four voices sound simultaneously, mixed to the master, audibly distinct.

**F4-AC6 — The 2-minute piece is possible.**
Given LAYERZ at the end of F4,
When the author produces a 2-minute musical piece using all four layers,
Then the author confirms the piece is musically complete without resorting to another tool. (Subjective gate; satisfied by signed-off recording in `docs/validation/`.)

## §F5 — Cross-cutting FX & spatial

**F5-AC1 — PULVERIZE alters the source signal destructively.**
Given any layer with a recognizable signal (e.g. BASS playing C2),
When PULVERIZE is engaged with smear = high, grain size = small, freeze = on,
Then the audible output is materially different from the source (verifiable by listening test and by spectral diff): the original signal is no longer identifiable as a clean C2 bass note.

**F5-AC2 — HARDPAN places signal off-center.**
Given a layer playing on the master bus,
When HARDPAN is set to -1.0 (full left),
Then the right-channel output of that layer is silent (below -90 dB) and the left channel carries the signal.

**F5-AC3 — PINGPONG alternates channels over time.**
Given a layer with PINGPONG enabled at the eighth-note rate,
When the pattern plays for one bar at 120 BPM,
Then the signal alternates between left-dominant and right-dominant on each eighth-note boundary, with the duty cycle matching configuration.

**F5-AC4 — AUTOPAN tracks LFO.**
Given AUTOPAN configured with LFO rate 0.5 Hz, sine shape, depth 100%,
When the layer plays,
Then the pan position oscillates smoothly between -1 and +1 with a 2-second period, verifiable by capturing the pan parameter automation.

**F5-AC5 — FLUX modulates a target parameter.**
Given FLUX configured with rate, depth, smoothing, routed to the cutoff of a HARMONIC layer's filter,
When the layer plays,
Then the cutoff value drifts within the configured range over time, audibly affecting the timbre, and the drift trajectory is non-repeating within a bar (proving random-walk, not LFO).

## §F6 — LLM bridge

**F6-AC1 — LLM bridge sends and receives.**
Given a valid API key configured and a current `Project` loaded,
When the user submits the prompt "fill the BEAT layer with a half-time pattern",
Then within a reasonable timeout (30 seconds), the bridge receives a response, parses it as a PatternPatch, and presents the patch for preview.

**F6-AC2 — Invalid patches are rejected.**
Given an LLM response that contains a PatternPatch with an out-of-range step (e.g. step = 99 in a 16-step pattern),
When validation runs,
Then the patch is rejected with a human-readable error, the raw response is preserved for inspection, and the `Project` is not modified.

**F6-AC3 — Patch preview reflects diff accurately.**
Given a valid PatternPatch that adds 3 events and removes 1,
When the preview is shown,
Then the UI clearly indicates which events would be added, which removed, and which unchanged, before the user clicks apply.

**F6-AC4 — Apply commits the patch.**
Given a previewed and accepted patch,
When the user clicks apply,
Then the `Project` is updated to the post-patch state, the change is reflected in the GUI within one frame, and the original pre-patch state is recoverable via undo.

**F6-AC5 — Canonical product independence.**
Given the LLM bridge generating a patch,
When the patch is examined,
Then it references only the `Project` model schema — never audio rendering details, never UI state, never a specific output mode. (This proves the canonical product abstraction holds: the LLM speaks only the model.)

## §F7 — Distribution

**F7-AC1 — Installable on each OS.**
Given the release artifacts for Linux, macOS, Windows,
When a user runs the platform installer on a clean machine,
Then LAYERZ installs and launches in both standalone and plugin form, with bundled presets and templates available.

**F7-AC2 — First-run experience.**
Given a brand-new install,
When the user opens LAYERZ for the first time,
Then the app loads a starter project demonstrating each layer with simple content, so the user hears something musical within 10 seconds of opening.

**F7-AC3 — Documentation is complete.**
Given the v1 release,
When a new user reads only the README and the user guide,
Then they can install LAYERZ, load it in a host, make a 16-step beat, apply DRAG, save and load a project, and apply a template with morph — without consulting external resources.

**F7-AC4 — Beta blockers resolved.**
Given the 3–5 beta producers' feedback,
When the v1 release is cut,
Then every issue tagged "blocker" by the author is either fixed or explicitly documented in known-limitations with a v2 link.

---

## Note on spikes

Spike phases (F0.99, F0.99b, F3.99, F4.99, F5.99, F6.99) do **not** have Given/When/Then acceptance criteria. Each spike's output is a written SUMMARY document committed to `docs/spikes/<phase>.md` containing: the question asked, the methods tried, the measurements collected, the decision reached, and the consequences for downstream phases. The phase that depends on the spike cannot begin its acceptance work until the SUMMARY exists.
