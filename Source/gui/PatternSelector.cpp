#include "PatternSelector.h"
#include "../PluginProcessor.h"

static const char* kLabels[] = { "A", "B", "C", "D" };

PatternSelector::PatternSelector(LayerzProcessor& proc) : proc_(proc) {
    for (int i = 0; i < 4; ++i) {
        addAndMakeVisible(patBtns_[i]);
        patBtns_[i].setButtonText(kLabels[i]);
        patBtns_[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(0xFF2A2A2Au));
        patBtns_[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFC0392Bu));
        patBtns_[i].setToggleable(true);

        patBtns_[i].onClick = [this, i] {
            proc_.chainManager().requestPatternSwitch(i);
            proc_.projectStore().postMutation([i](Project& p) {
                p.active_pattern_index = juce::jlimit(0, (int)p.patterns.size()-1, i);
            });
        };
    }
    startTimerHz(10);
}

PatternSelector::~PatternSelector() { stopTimer(); }

void PatternSelector::resized() {
    auto b = getLocalBounds();
    int w = b.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        patBtns_[i].setBounds(b.getX() + i * w, b.getY(), w, b.getHeight());
}

void PatternSelector::timerCallback() {
    int playing = proc_.chainManager().currentPatternIndex();
    auto snap   = proc_.projectStore().snapshot();
    int numPat  = static_cast<int>(snap->patterns.size());
    for (int i = 0; i < 4; ++i) {
        bool exists  = i < numPat;
        bool active  = (i == playing);
        patBtns_[i].setVisible(exists);
        patBtns_[i].setToggleState(active, juce::dontSendNotification);
    }
}
