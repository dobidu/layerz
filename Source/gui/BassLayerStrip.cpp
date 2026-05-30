#include "BassLayerStrip.h"
#include "../PluginProcessor.h"

static const juce::Colour kAmber(0xFFE67E22u);
static const juce::Colour kBg   (0xFF181818u);
static const juce::Colour kSep  (0xFF2E2E2Eu);

BassLayerStrip::BassLayerStrip(LayerzProcessor& proc)
    : proc_(proc), stepRow_(proc.projectStore())
{
    addAndMakeVisible(stepRow_);
    startTimerHz(30);
}

BassLayerStrip::~BassLayerStrip() { stopTimer(); }

void BassLayerStrip::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    // Header
    g.setColour(kBg.brighter(0.08f));
    g.fillRect(0, 0, getWidth(), kHeaderH);
    g.setColour(kAmber);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    g.drawText("BASS", 8, 0, 60, kHeaderH, juce::Justification::centredLeft);
    g.setColour(kSep);
    g.drawHorizontalLine(kHeaderH - 1, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void BassLayerStrip::resized() {
    stepRow_.setBounds(0, kHeaderH, getWidth(), getHeight() - kHeaderH);
}

void BassLayerStrip::timerCallback() {
    auto snap = proc_.projectStore().snapshot();
    if (snap->patterns.empty()) return;
    stepRow_.updateFromSnapshot(*snap);
    stepRow_.setCurrentStep(proc_.currentPlayStep());
}
