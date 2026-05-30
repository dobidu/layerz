#include "PluginProcessor.h"
#include "PluginEditor.h"

LayerzProcessor::LayerzProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    config_  = UserConfig::load();
    profile_ = ProfileConfig::forProfile(static_cast<Profile>(config_.profile_override));
    seedTestPattern();
}

LayerzProcessor::~LayerzProcessor() {}

void LayerzProcessor::seedTestPattern() {
    store_.postMutation([](Project& p) {
        p.patterns.clear();
        Pattern pat;
        pat.id           = "default";
        pat.length_steps = 16;

        Layer beatLayer;
        beatLayer.type = LayerType::BEAT;

        // kick: quarter beats 0,4,8,12
        DrumTrack kick;
        kick.voice_type = "kick"; kick.param1 = 120.0f;  // pitch_decay_ms
        for (int s : {0, 4, 8, 12}) {
            Event e; e.step = s; e.velocity = 1.0f;
            kick.events.push_back(e);
        }

        // snare: beats 4, 12
        DrumTrack snare;
        snare.voice_type = "snare"; snare.param1 = 60.0f;   // noise_decay_ms
        for (int s : {4, 12}) {
            Event e; e.step = s; e.velocity = 0.85f;
            snare.events.push_back(e);
        }

        // hat: every even step
        DrumTrack hat;
        hat.voice_type = "hat"; hat.param1 = 25.0f;    // decay_ms (short, tight)
        for (int s = 0; s < 16; s += 2) {
            Event e; e.step = s; e.velocity = 0.6f;
            hat.events.push_back(e);
        }

        // perc: off-beats 2, 10
        DrumTrack perc;
        perc.voice_type = "perc"; perc.param1 = 200.0f;
        for (int s : {2, 10}) {
            Event e; e.step = s; e.velocity = 0.7f;
            perc.events.push_back(e);
        }

        beatLayer.drum_tracks.push_back(kick);
        beatLayer.drum_tracks.push_back(snare);
        beatLayer.drum_tracks.push_back(hat);
        beatLayer.drum_tracks.push_back(perc);
        pat.layers.push_back(beatLayer);

        // BASS layer: C2/E2/G2/Bb2 on quarter beats
        Layer bassLayer;
        bassLayer.type = LayerType::BASS;
        const int bassNotes[] = { 60, 64, 67, 70 }; // C4, E4, G4, Bb4 — within laptop speaker range
        const int bassSteps[] = { 0, 4, 8, 12 };
        for (int i = 0; i < 4; ++i) {
            Event e; e.step = bassSteps[i]; e.velocity = 0.8f; e.midi_note = bassNotes[i];
            bassLayer.events.push_back(e);
        }
        pat.layers.push_back(bassLayer);
        p.patterns.push_back(pat);
    });
}

void LayerzProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    voiceBank_.prepare(sampleRate, samplesPerBlock);
    sequencer_.prepare(sampleRate);
    clock_.prepare(sampleRate, samplesPerBlock);

    // wrapperType is set by JUCE before constructor — reliable standalone detection
    bool standalone = (wrapperType == wrapperType_Standalone);
    clock_.setStandaloneMode(standalone);
    if (standalone) clock_.setPlaying(true);
}

void LayerzProcessor::releaseResources() {}

void LayerzProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
    AudioThreadGuard guard;
    buffer.clear();

    // Lock-free atomic snapshot — use for BOTH sequencer AND MIDI handler (audit M1)
    auto snap = store_.snapshot();

    // Pre-extract BASS params ONCE before MIDI loop (audit M1+M2)
    BassVoiceParams midiBassPar;
    if (!snap->patterns.empty()) {
        int pi = juce::jlimit(0, (int)snap->patterns.size()-1, snap->active_pattern_index);
        for (const auto& l : snap->patterns[static_cast<std::size_t>(pi)].layers)
            if (l.type == LayerType::BASS) { midiBassPar = l.bass_params; break; }
    }

    // MIDI routing — uses snap already acquired above (no second snapshot() call)
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        if (msg.isNoteOn()) {
            voiceBank_.releaseBass();
            voiceBank_.triggerBass(msg.getNoteNumber(), msg.getVelocity() / 127.0f, midiBassPar);
            voiceBank_.processBass(buffer, samplePos, buffer.getNumSamples() - samplePos);
        } else if (msg.isNoteOff()) {
            voiceBank_.releaseBass();
        }
    }

    juce::AudioPlayHead::PositionInfo pos;
    if (auto* ph = getPlayHead())
        if (auto p = ph->getPosition())
            pos = *p;

    auto events = clock_.process(pos, buffer.getNumSamples());
    sequencer_.process(*snap, events, buffer, voiceBank_);

    // Copy ch0 → ch1 for stereo output (voices write mono to ch0 only)
    if (buffer.getNumChannels() >= 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    // Track current step for UI step indicator (audio thread → atomic write)
    if (events.count > 0 && !snap->patterns.empty() && !snap->patterns[0].layers.empty()) {
        int len = snap->patterns[0].length_steps;
        lastPlayStep_.store(events.data[events.count - 1].beat_index % len,
                            std::memory_order_relaxed);
    } else if (!clock_.isPlaying()) {
        lastPlayStep_.store(-1, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* LayerzProcessor::createEditor() {
    return new LayerzEditor(*this);
}

void LayerzProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto json = ProjectStore::toJson(*store_.snapshot());
    destData.replaceAll(json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void LayerzProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto json = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    Project loaded;
    if (ProjectStore::fromJson(json, loaded).wasOk()) {
        // Check if loaded project has a BASS layer; if not, seed one
        bool hasBass = false;
        for (const auto& pat : loaded.patterns)
            for (const auto& l : pat.layers)
                if (l.type == LayerType::BASS) { hasBass = true; break; }

        store_.postMutation([&loaded](Project& p) { p = loaded; });

        if (! hasBass)
            seedTestPattern(); // re-adds BASS layer to the first pattern
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LayerzProcessor();
}
