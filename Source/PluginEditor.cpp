#include "PluginEditor.h"

LayerzEditor::LayerzEditor(LayerzProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(900, 600);
}

LayerzEditor::~LayerzEditor() {}

void LayerzEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("LAYERZ", getLocalBounds(), juce::Justification::centred, true);
}

void LayerzEditor::resized() {}
