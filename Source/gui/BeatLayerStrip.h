#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "StepRow.h"
#include "VoiceParamPanel.h"

// Forward declaration to avoid circular includes
class LayerzProcessor;

// BEAT layer strip: 4 rows of (VoiceParamPanel + StepRow).
// Timer at 30fps syncs UI state from ProjectStore snapshot.
class BeatLayerStrip : public juce::Component, public juce::Timer {
public:
    explicit BeatLayerStrip(LayerzProcessor& proc);
    ~BeatLayerStrip() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    LayerzProcessor& proc_;
    StepRow         stepRows_[4];
    VoiceParamPanel paramPanels_[4];

    static constexpr int kParamPanelWidth = 120;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeatLayerStrip)
};
