#include "PluginProcessor.h"
#include "PluginEditor.h"

LayerzProcessor::LayerzProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

LayerzProcessor::~LayerzProcessor() {}

void LayerzProcessor::prepareToPlay(double, int) {}

void LayerzProcessor::releaseResources() {}

void LayerzProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    buffer.clear();
}

juce::AudioProcessorEditor* LayerzProcessor::createEditor() {
    return new LayerzEditor(*this);
}

void LayerzProcessor::getStateInformation(juce::MemoryBlock&) {}

void LayerzProcessor::setStateInformation(const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LayerzProcessor();
}
