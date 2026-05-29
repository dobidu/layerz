#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/ProjectStore.h"

// 16-button step row for one drum track.
// onClick: postMutation to add/remove Event. Optimistic visual update.
// updateFromSnapshot: called by BeatLayerStrip timer — uses dontSendNotification.
class StepRow : public juce::Component {
public:
    StepRow(ProjectStore& store, int trackIndex);

    void paint(juce::Graphics&) override;
    void resized() override;
    void updateFromSnapshot(const Project& snap);
    void setCurrentStep(int step) noexcept;  // -1 = not playing

private:
    ProjectStore& store_;
    int           trackIndex_;
    int           currentStep_ = -1;
    juce::TextButton stepButtons_[16];

    bool hasEventAtStep(const Project& snap, int step) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepRow)
};
