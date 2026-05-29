#include "BeatLayerStrip.h"
#include "../PluginProcessor.h"

static constexpr int kRowHeight      = 70;
static constexpr int kHeaderHeight   = 24;
static constexpr int kParamWidth     = 96;
static const juce::Colour kCrimson   (0xFFC0392Bu);
static const juce::Colour kBg        (0xFF181818u);
static const juce::Colour kRowBg     (0xFF1E1E1Eu);
static const juce::Colour kSep       (0xFF2E2E2Eu);

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

BeatLayerStrip::~BeatLayerStrip() { stopTimer(); }

void BeatLayerStrip::paint(juce::Graphics& g) {
    g.fillAll(kBg);

    // Header: "BEAT" label
    g.setColour(kBg.brighter(0.08f));
    g.fillRect(0, 0, getWidth(), kHeaderHeight);
    g.setColour(kCrimson);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    g.drawText("BEAT", 8, 0, 60, kHeaderHeight, juce::Justification::centredLeft);
    g.setColour(kSep);
    g.drawHorizontalLine(kHeaderHeight - 1, 0.0f, static_cast<float>(getWidth()));

    // Alternating row backgrounds + separators
    for (int i = 0; i < 4; ++i) {
        int y = kHeaderHeight + i * kRowHeight;
        g.setColour(i % 2 == 0 ? kRowBg : kBg);
        g.fillRect(0, y, getWidth(), kRowHeight);
        g.setColour(kSep);
        g.drawHorizontalLine(y + kRowHeight - 1, 0.0f, static_cast<float>(getWidth()));
    }

    // Left panel separator
    g.setColour(kSep.brighter(0.2f));
    g.drawVerticalLine(kParamWidth, kHeaderHeight, static_cast<float>(getHeight()));
}

void BeatLayerStrip::resized() {
    for (int i = 0; i < 4; ++i) {
        int y = kHeaderHeight + i * kRowHeight;
        paramPanels_[i].setBounds(0, y, kParamWidth, kRowHeight);
        stepRows_[i].setBounds(kParamWidth, y, getWidth() - kParamWidth, kRowHeight);
    }
}

void BeatLayerStrip::timerCallback() {
    auto snap = proc_.projectStore().snapshot();
    int playStep = proc_.currentPlayStep();
    for (int i = 0; i < 4; ++i) {
        stepRows_[i].updateFromSnapshot(*snap);
        stepRows_[i].setCurrentStep(playStep);
        paramPanels_[i].updateFromSnapshot(*snap);
    }
}
