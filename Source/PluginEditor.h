#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class LayerzEditor : public juce::AudioProcessorEditor {
public:
    explicit LayerzEditor(LayerzProcessor&);
    ~LayerzEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerzEditor)
};
