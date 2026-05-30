#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../model/ProjectStore.h"

// Template selector buttons + morph knob for one layer.
// template buttons: click → sets layer.template_name + applies to layer.aesthetics via morph
// morph slider: 0=neutral → 1=full template
class GroovePanel : public juce::Component {
public:
    GroovePanel(ProjectStore& store, LayerType layerType);
    void resized() override;
    void updateFromSnapshot(const Project& snap);

private:
    ProjectStore& store_;
    LayerType     layerType_;
    juce::TextButton templateBtns_[8];
    int              numTemplates_ = 0;
    juce::Slider     morphSlider_;
    juce::Label      label_;

    // Find this layer in the active pattern
    static Layer* findLayer(Project& p, LayerType t);
    static const Layer* findLayerConst(const Project& p, LayerType t);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroovePanel)
};
