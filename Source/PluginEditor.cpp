#include "PluginEditor.h"

static constexpr int kTopBarHeight  = 48;
static constexpr int kLayerRowH     = 26;  // placeholder rows for future layers
static constexpr int kBeatStripH    = 280; // 4 rows × 70px

LayerzEditor::LayerzEditor(LayerzProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef_(p)
    , beatStrip_(p)
{
    addAndMakeVisible(beatStrip_);

    // wrapperType is reliable before prepareToPlay; isStandaloneMode() is not (set in prepareToPlay)
    bool standalone = (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    // BPM slider — standalone only
    addChildComponent(bpmSlider_);
    bpmSlider_.setSliderStyle(juce::Slider::IncDecButtons);
    bpmSlider_.setRange(60.0, 200.0, 1.0);
    bpmSlider_.setValue(120.0, juce::dontSendNotification);
    bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, kTopBarHeight - 8);
    bpmSlider_.onValueChange = [this] {
        processorRef_.getClock().setBpm(bpmSlider_.getValue());
    };
    bpmSlider_.setVisible(standalone);

    // Play button — standalone only
    addChildComponent(playButton_);
    playButton_.setButtonText("PLAY");
    playButton_.setToggleable(true);
    playButton_.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF2A2A2Au));
    playButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF27AE60u));
    playButton_.onClick = [this] {
        bool playing = playButton_.getToggleState();
        processorRef_.setStandalonePlaying(playing);
        playButton_.setButtonText(playing ? "■ STOP" : "▶ PLAY");
    };
    playButton_.setVisible(standalone);

    setSize(900, 600);
}

LayerzEditor::~LayerzEditor() {}

void LayerzEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF121212u));

    // ── Top bar ──────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xFF1C1C1Cu));
    g.fillRect(0, 0, getWidth(), kTopBarHeight);
    g.setColour(juce::Colour(0xFF333333u));
    g.drawHorizontalLine(kTopBarHeight, 0.0f, static_cast<float>(getWidth()));

    // LAYERZ title
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(18.0f)));
    g.setColour(juce::Colour(0xFFC0392Bu));
    g.drawText("LAYERZ", 12, 0, 100, kTopBarHeight, juce::Justification::centredLeft);

    // Plugin-mode transport hint
    if (! processorRef_.getClock().isStandaloneMode()) {
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        g.setColour(juce::Colour(0xFF666666u));
        g.drawText("Press Play in host to start", 120, 0, 260, kTopBarHeight,
                   juce::Justification::centredLeft);
    }

    // ── Layer labels (placeholder for BASS/HARMONIC/MELODIC) ─────────────────
    int y = kTopBarHeight + kBeatStripH + 8;
    const char* labels[] = { "BASS", "HARMONIC", "MELODIC" };
    const juce::Colour colours[] = {
        juce::Colour(0xFF996B00u),  // amber
        juce::Colour(0xFF006B6Bu),  // teal
        juce::Colour(0xFF5B006Bu)   // violet
    };
    for (int i = 0; i < 3; ++i) {
        g.setColour(juce::Colour(0xFF1A1A1Au));
        g.fillRect(0, y, getWidth(), kLayerRowH);
        g.setColour(colours[i].withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        g.drawText(juce::String(labels[i]) + "  —  coming in F2/F4",
                   12, y, 300, kLayerRowH, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xFF2A2A2Au));
        g.drawHorizontalLine(y + kLayerRowH - 1, 0.0f, static_cast<float>(getWidth()));
        y += kLayerRowH;
    }
}

void LayerzEditor::resized() {
    int y = kTopBarHeight;
    if (processorRef_.getClock().isStandaloneMode()) {
        auto topBar = juce::Rectangle<int>(0, 0, getWidth(), kTopBarHeight);
        bpmSlider_.setBounds (topBar.withTrimmedLeft(120).withWidth(120).reduced(4));
        playButton_.setBounds(topBar.withTrimmedLeft(250).withWidth(80).reduced(4));
    }
    beatStrip_.setBounds(0, y, getWidth(), kBeatStripH);
}
