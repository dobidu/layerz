#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

static constexpr float kClickFreq = 440.0f;

LayerzProcessor::LayerzProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    config_  = UserConfig::load();
    profile_ = ProfileConfig::forProfile(
        static_cast<Profile>(config_.profile_override));
    buildClickBuffer();
}

LayerzProcessor::~LayerzProcessor() {}

void LayerzProcessor::buildClickBuffer() noexcept {
    for (int i = 0; i < kClickSamples; ++i) {
        float window = 0.5f * (1.0f - std::cos(
            2.0f * juce::MathConstants<float>::pi * i / (kClickSamples - 1)));
        float sine   = std::sin(
            2.0f * juce::MathConstants<float>::pi * kClickFreq * i / 44100.0f);
        clickBuffer_[i] = window * sine * 0.3f;  // -10 dB to avoid clipping
    }
}

void LayerzProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    clock_.prepare(sampleRate, samplesPerBlock);

    // Standalone mode: detected via absence of a playhead position
    bool standalone = (getPlayHead() == nullptr);
    clock_.setStandaloneMode(standalone);
    if (standalone) clock_.setPlaying(true);

    // Recalculate click buffer with actual sample rate
    for (int i = 0; i < kClickSamples; ++i) {
        float window = 0.5f * (1.0f - std::cos(
            2.0f * juce::MathConstants<float>::pi * i / (kClickSamples - 1)));
        float sine   = std::sin(
            2.0f * juce::MathConstants<float>::pi * kClickFreq * i
            / static_cast<float>(sampleRate));
        clickBuffer_[i] = window * sine * 0.3f;
    }
}

void LayerzProcessor::releaseResources() {}

void LayerzProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    buffer.clear();

    // Snapshot current project state — O(1), short lock, no allocation
    [[maybe_unused]] auto snap = store_.snapshot();

    // Get host position (nullptr in standalone)
    juce::AudioPlayHead::PositionInfo pos;
    if (auto* ph = getPlayHead())
        if (auto p = ph->getPosition())
            pos = *p;

    // Get beat events for this block — no allocation
    auto events = clock_.process(pos, buffer.getNumSamples());

    // Write Hann-windowed 440 Hz click at each beat offset (channel 0)
    // This is the F0 timing proof signal — not a musical feature
    if (buffer.getNumChannels() > 0) {
        auto* ch = buffer.getWritePointer(0);
        for (const auto& ev : events) {
            int start = ev.sample_offset;
            int avail = buffer.getNumSamples() - start;
            int n     = juce::jmin(kClickSamples, avail);
            for (int i = 0; i < n; ++i)
                ch[start + i] += clickBuffer_[i];
        }
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
    auto json = juce::String::fromUTF8(static_cast<const char*>(data),
                                       sizeInBytes);
    Project loaded;
    if (ProjectStore::fromJson(json, loaded).wasOk())
        store_.postMutation([&loaded](Project& p) { p = loaded; });
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LayerzProcessor();
}
