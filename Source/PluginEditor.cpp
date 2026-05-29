#include "PluginEditor.h"

static constexpr int kTopBarHeight = 40;

LayerzEditor::LayerzEditor(LayerzProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef_(p)
    , beatStrip_(p)
{
    addAndMakeVisible(beatStrip_);

    // Standalone transport controls
    bool standalone = p.getClock().isStandaloneMode();

    addChildComponent(bpmSlider_);
    bpmSlider_.setSliderStyle(juce::Slider::IncDecButtons);
    bpmSlider_.setRange(60.0, 200.0, 1.0);
    bpmSlider_.setValue(120.0, juce::dontSendNotification);
    bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, kTopBarHeight);
    bpmSlider_.onValueChange = [this] {
        processorRef_.getClock().setBpm(bpmSlider_.getValue());
    };
    bpmSlider_.setVisible(standalone);

    addChildComponent(playButton_);
    playButton_.setButtonText("PLAY");
    playButton_.setToggleable(true);
    playButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2Au));
    playButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF27AE60u));
    playButton_.onClick = [this] {
        bool playing = playButton_.getToggleState();
        processorRef_.setStandalonePlaying(playing);
        playButton_.setButtonText(playing ? "STOP" : "PLAY");
    };
    playButton_.setVisible(standalone);

    setSize(900, 600);
}

LayerzEditor::~LayerzEditor() {}

void LayerzEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF121212u));
}

void LayerzEditor::resized() {
    auto b = getLocalBounds();
    bool standalone = processorRef_.getClock().isStandaloneMode();

    if (standalone) {
        auto topBar = b.removeFromTop(kTopBarHeight);
        bpmSlider_.setBounds (topBar.removeFromLeft(120));
        playButton_.setBounds(topBar.removeFromLeft(80));
    }

    beatStrip_.setBounds(b);
}
