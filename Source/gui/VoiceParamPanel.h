#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/ProjectStore.h"

// Per-drum-track controls: voice label, level slider, mute button, param1 slider.
// All onChange callbacks use juce::dontSendNotification in updateFromSnapshot
// to prevent infinite postMutation loops.
class VoiceParamPanel : public juce::Component {
public:
    VoiceParamPanel(ProjectStore& store, int trackIndex);

    void resized() override;
    void updateFromSnapshot(const Project& snap);

private:
    ProjectStore&    store_;
    int              trackIndex_;
    juce::Label      voiceLabel_;
    juce::Slider     levelSlider_;
    juce::TextButton muteButton_;
    juce::Slider     param1Slider_;

    static const char* voiceName(int idx) noexcept;
    static const char* param1Label(int idx) noexcept;
    static float param1Min(int idx) noexcept;
    static float param1Max(int idx) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceParamPanel)
};
