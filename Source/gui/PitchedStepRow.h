#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/ProjectStore.h"

// 16 step buttons for BASS layer. Each active step shows its note name.
// Click inactive → popup to pick note. Click active → remove.
class PitchedStepRow : public juce::Component {
public:
    explicit PitchedStepRow(ProjectStore& store);
    void paint(juce::Graphics&) override;
    void resized() override;
    void updateFromSnapshot(const Project& snap);
    void setCurrentStep(int step) noexcept;

private:
    ProjectStore& store_;
    int currentStep_ = -1;
    juce::TextButton stepButtons_[16];

    // Finds BASS layer by type (NOT by index)
    const Layer* findBassLayer(const Project& snap) const;
    bool hasEventAtStep(const Project& snap, int step) const;
    int  noteAtStep   (const Project& snap, int step) const;

    static juce::String noteName(int midi) {
        static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        int octave = midi / 12 - 1;
        return juce::String(names[midi % 12]) + juce::String(octave);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchedStepRow)
};
