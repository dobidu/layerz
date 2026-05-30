#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PitchedStepRow.h"
#include "GroovePanel.h"

class LayerzProcessor;

// BASS layer strip — amber accent, single pitched step row.
class BassLayerStrip : public juce::Component, public juce::Timer {
public:
    explicit BassLayerStrip(LayerzProcessor& proc);
    ~BassLayerStrip() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    LayerzProcessor& proc_;
    PitchedStepRow   stepRow_;
    GroovePanel      groovePanel_;
    static constexpr int kHeaderH = 24;
    static constexpr int kGrooveH = 36;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassLayerStrip)
};
