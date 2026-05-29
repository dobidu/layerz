#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "gui/BeatLayerStrip.h"

class LayerzEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit LayerzEditor(LayerzProcessor&);
    ~LayerzEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override; // syncs play button state with clock

private:
    LayerzProcessor& processorRef_;
    BeatLayerStrip   beatStrip_;

    juce::Slider     bpmSlider_;   // standalone only
    juce::TextButton playButton_;  // standalone only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerzEditor)
};
