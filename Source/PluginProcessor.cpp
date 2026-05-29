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
        kick.voice_type = "kick"; kick.param1 = 80.0f;
        for (int s : {0, 4, 8, 12}) {
            Event e; e.step = s; e.velocity = 1.0f;
            kick.events.push_back(e);
        }

        // snare: beats 4, 12
        DrumTrack snare;
        snare.voice_type = "snare"; snare.param1 = 80.0f;
        for (int s : {4, 12}) {
            Event e; e.step = s; e.velocity = 0.85f;
            snare.events.push_back(e);
        }

        // hat: every even step
        DrumTrack hat;
        hat.voice_type = "hat"; hat.param1 = 30.0f;
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
    AudioThreadGuard guard;  // marks audio thread active for RT safety checks
    juce::ignoreUnused(midiMessages);
    buffer.clear();

    // Lock-free atomic snapshot — zero user-level locks on audio thread (F1-AC4)
    auto snap = store_.snapshot();

    juce::AudioPlayHead::PositionInfo pos;
    if (auto* ph = getPlayHead())
        if (auto p = ph->getPosition())
            pos = *p;

    auto events = clock_.process(pos, buffer.getNumSamples());
    sequencer_.process(*snap, events, buffer, voiceBank_);
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
    if (ProjectStore::fromJson(json, loaded).wasOk())
        store_.postMutation([&loaded](Project& p) { p = loaded; });
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LayerzProcessor();
}
