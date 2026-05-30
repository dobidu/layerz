#include "AestheticResolver.h"
#include <cmath>

void AestheticResolver::prepare(double sampleRate, int maxBlockSize) noexcept {
    sampleRate_ = sampleRate;
    blockSize_  = maxBlockSize;
    pendingCount_ = 0;
    lcgState_   = 12345u;
}

int AestheticResolver::drainPending(ResolvedEvent* out, int maxOut) noexcept {
    int count = juce::jmin(pendingCount_, maxOut);
    for (int i = 0; i < count; ++i)
        out[i] = pendingQueue_[i];
    // Shift remaining
    int remaining = pendingCount_ - count;
    for (int i = 0; i < remaining; ++i)
        pendingQueue_[i] = pendingQueue_[count + i];
    pendingCount_ = remaining;
    return count;
}

bool AestheticResolver::enqueue(const ResolvedEvent& ev) noexcept {
    if (pendingCount_ >= kQueueCap) return false;
    pendingQueue_[pendingCount_++] = ev;
    return true;
}

int AestheticResolver::emitOne(ResolvedEvent ev, int blockSize,
                                ResolvedEvent* out, int maxOut, int& count) noexcept {
    if (ev.sample_offset < 0)
        ev.sample_offset = juce::jmax(0, ev.sample_offset); // PUSH clamp (audit M1 fix)
    if (ev.sample_offset >= blockSize) {
        ev.sample_offset -= blockSize;  // DRAG carry-forward
        enqueue(ev);
        return 0;
    }
    if (count < maxOut)
        out[count++] = ev;
    return 1;
}

int AestheticResolver::resolveEvent(bool isBass,
                                     const std::string& voiceType,
                                     const Event& event,
                                     const Aesthetics& aes,
                                     const BassVoiceParams* bassParams,
                                     float baseVelocity,
                                     float param1,
                                     int beatSampleOffset,
                                     int blockSize,
                                     double stepPeriodSamples,
                                     ResolvedEvent* out, int maxOut) noexcept {
    // 1. FRACTURE check — drop event probabilistically before any other processing
    if (aes.fracture_prob > 0.0f) {
        float roll = static_cast<float>(nextLCG() % 1000u) / 1000.0f;
        if (roll >= aes.fracture_prob) return 0; // event dropped
    }

    // 2. Compute displaced base offset (DRAG + PUSH)
    int drag_samples = static_cast<int>(aes.drag * stepPeriodSamples * 0.5);  // max 50% step
    int push_samples = static_cast<int>(aes.push * stepPeriodSamples * 0.3);  // max 30% step
    int baseOffset = beatSampleOffset + drag_samples - push_samples;

    // Build base event template
    ResolvedEvent base;
    base.midi_note    = event.midi_note;
    base.velocity     = baseVelocity;
    base.sample_offset= baseOffset;
    base.is_bass      = isBass;
    base.voice_type   = voiceType;
    base.param1       = param1;
    base.bass_params  = bassParams;

    int count = 0;

    // 3. ROLL — subdivide step into roll_mult sub-events
    if (aes.roll_mult > 1) {
        int spacing = juce::jmax(1, static_cast<int>(stepPeriodSamples / aes.roll_mult));
        for (int k = 0; k < aes.roll_mult; ++k) {
            ResolvedEvent ev = base;
            ev.sample_offset = baseOffset + k * spacing;
            ev.velocity      = base.velocity * std::pow(aes.roll_vel_decay, static_cast<float>(k));
            emitOne(ev, blockSize, out, maxOut, count);
        }
        return count;
    }

    // 4. STUTTER — emit stutter_reps gated repetitions
    if (aes.stutter_reps > 0) {
        int gate = juce::jmax(1, static_cast<int>(aes.stutter_gate * stepPeriodSamples));
        for (int k = 0; k < aes.stutter_reps; ++k) {
            ResolvedEvent ev = base;
            ev.sample_offset = baseOffset + k * gate;
            // Mark as stutter sub-event for VoiceBank (uses triggerRetain for k>0)
            if (k > 0) ev.velocity = -ev.velocity; // negative = retrigger flag (no phase reset)
            emitOne(ev, blockSize, out, maxOut, count);
        }
        return count;
    }

    // 5. Plain event with displacement
    emitOne(base, blockSize, out, maxOut, count);
    return count;
}
