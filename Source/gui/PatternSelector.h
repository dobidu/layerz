#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/ProjectStore.h"

class LayerzProcessor;

// A/B/C/D pattern selection buttons.
// Clicking switches to that pattern at the next bar boundary.
class PatternSelector : public juce::Component, public juce::Timer {
public:
    explicit PatternSelector(LayerzProcessor& proc);
    ~PatternSelector() override;
    void resized() override;
    void timerCallback() override;

private:
    LayerzProcessor& proc_;
    juce::TextButton patBtns_[4];  // A / B / C / D

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSelector)
};
