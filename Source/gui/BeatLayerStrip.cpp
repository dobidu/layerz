#include "BeatLayerStrip.h"
#include "../PluginProcessor.h"

BeatLayerStrip::BeatLayerStrip(LayerzProcessor& proc)
    : proc_(proc)
    , stepRows_   { StepRow(proc.projectStore(), 0),
                    StepRow(proc.projectStore(), 1),
                    StepRow(proc.projectStore(), 2),
                    StepRow(proc.projectStore(), 3) }
    , paramPanels_{ VoiceParamPanel(proc.projectStore(), 0),
                    VoiceParamPanel(proc.projectStore(), 1),
                    VoiceParamPanel(proc.projectStore(), 2),
                    VoiceParamPanel(proc.projectStore(), 3) }
{
    for (int i = 0; i < 4; ++i) {
        addAndMakeVisible(stepRows_[i]);
        addAndMakeVisible(paramPanels_[i]);
    }
    startTimerHz(30);
}

BeatLayerStrip::~BeatLayerStrip() {
    stopTimer();
}

void BeatLayerStrip::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF1A1A1Au));
    // Row separators
    auto b = getLocalBounds();
    int rowH = b.getHeight() / 4;
    g.setColour(juce::Colour(0xFF333333u));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine(rowH * i, 0.0f, static_cast<float>(b.getWidth()));
    // BEAT label
    g.setColour(juce::Colour(0xFFC0392Bu));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f)));
}

void BeatLayerStrip::resized() {
    auto b = getLocalBounds();
    int rowH = b.getHeight() / 4;
    for (int i = 0; i < 4; ++i) {
        int y = b.getY() + i * rowH;
        paramPanels_[i].setBounds(b.getX(), y, kParamPanelWidth, rowH);
        stepRows_[i].setBounds(b.getX() + kParamPanelWidth, y,
                               b.getWidth() - kParamPanelWidth, rowH);
    }
}

void BeatLayerStrip::timerCallback() {
    auto snap = proc_.projectStore().snapshot();
    for (int i = 0; i < 4; ++i) {
        stepRows_[i].updateFromSnapshot(*snap);
        paramPanels_[i].updateFromSnapshot(*snap);
    }
}
