#include "GroovePanel.h"

static const juce::Colour kCrimson(0xFFC0392Bu);
static const juce::Colour kAmber  (0xFFE67E22u);
static const juce::Colour kDark   (0xFF2A2A2Au);

Layer* GroovePanel::findLayer(Project& p, LayerType t) {
    if (p.patterns.empty()) return nullptr;
    int pi = juce::jlimit(0,(int)p.patterns.size()-1, p.active_pattern_index);
    for (auto& l : p.patterns[static_cast<std::size_t>(pi)].layers)
        if (l.type == t) return &l;
    return nullptr;
}
const Layer* GroovePanel::findLayerConst(const Project& p, LayerType t) {
    if (p.patterns.empty()) return nullptr;
    int pi = juce::jlimit(0,(int)p.patterns.size()-1, p.active_pattern_index);
    for (const auto& l : p.patterns[static_cast<std::size_t>(pi)].layers)
        if (l.type == t) return &l;
    return nullptr;
}

GroovePanel::GroovePanel(ProjectStore& store, LayerType layerType)
    : store_(store), layerType_(layerType)
{
    juce::Colour accent = (layerType_ == LayerType::BEAT) ? kCrimson : kAmber;

    const TemplateDef* table = (layerType_ == LayerType::BEAT) ? kBeatTemplates : kBassTemplates;
    numTemplates_ = (layerType_ == LayerType::BEAT) ? kNumBeatTemplates : kNumBassTemplates;

    for (int i = 0; i < numTemplates_; ++i) {
        addAndMakeVisible(templateBtns_[i]);
        templateBtns_[i].setButtonText(table[i].name);
        templateBtns_[i].setToggleable(true);
        templateBtns_[i].setColour(juce::TextButton::buttonColourId,   kDark);
        templateBtns_[i].setColour(juce::TextButton::buttonOnColourId, accent);
        templateBtns_[i].setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
        std::string tName = table[i].name;
        templateBtns_[i].onClick = [this, tName] {
            store_.postMutation([this, tName](Project& p) {
                if (auto* l = findLayer(p, layerType_))
                    l->template_name = tName;
            });
        };
    }

    addAndMakeVisible(morphSlider_);
    morphSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    morphSlider_.setRange(0.0, 1.0, 0.0);
    morphSlider_.setValue(0.0, juce::dontSendNotification);
    morphSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    morphSlider_.onValueChange = [this] {
        float v = static_cast<float>(morphSlider_.getValue());
        store_.postMutation([this, v](Project& p) {
            if (auto* l = findLayer(p, layerType_))
                l->morph_amount = v;
        });
    };

    addAndMakeVisible(label_);
    label_.setText("MORPH", juce::dontSendNotification);
    label_.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    label_.setColour(juce::Label::textColourId, juce::Colour(0xFF666666u));
    label_.setJustificationType(juce::Justification::centredLeft);
}

void GroovePanel::resized() {
    auto b = getLocalBounds();
    // Label on far left
    label_.setBounds(b.removeFromLeft(40).reduced(2));
    // Morph slider on far right
    auto morphArea = b.removeFromRight(100).reduced(2);
    morphSlider_.setBounds(morphArea);
    // Template buttons fill remaining space
    int btnW = (b.getWidth()) / juce::jmax(1, numTemplates_);
    for (int i = 0; i < numTemplates_; ++i)
        templateBtns_[i].setBounds(b.getX() + i * btnW, b.getY(), btnW, b.getHeight());
}

void GroovePanel::updateFromSnapshot(const Project& snap) {
    const Layer* l = findLayerConst(snap, layerType_);
    if (! l) return;
    morphSlider_.setValue(static_cast<double>(l->morph_amount), juce::dontSendNotification);
    for (int i = 0; i < numTemplates_; ++i) {
        const TemplateDef* table = (layerType_ == LayerType::BEAT) ? kBeatTemplates : kBassTemplates;
        bool active = (l->template_name == table[i].name);
        templateBtns_[i].setToggleState(active, juce::dontSendNotification);
    }
}
