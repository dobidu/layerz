#include "PluginEditor.h"

LayerzEditor::LayerzEditor(LayerzProcessor& p)
    : AudioProcessorEditor(&p)
{
    setSize(900, 600);
}

LayerzEditor::~LayerzEditor() {}

void LayerzEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(18.0f)));
    g.drawText("LAYERZ", getLocalBounds(), juce::Justification::centred, true);
}

void LayerzEditor::resized() {}
