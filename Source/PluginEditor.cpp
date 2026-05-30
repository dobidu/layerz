#include "PluginEditor.h"

static constexpr int kTopBarHeight  = 48;
static constexpr int kLayerRowH     = 26;  // placeholder rows for future layers
static constexpr int kBeatStripH    = 304; // 24px header + 4×70px rows

LayerzEditor::LayerzEditor(LayerzProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef_(p)
    , beatStrip_(p)
    , patternSelector_(p)
    , bassStrip_(p)
{
    addAndMakeVisible(beatStrip_);
    addAndMakeVisible(bassStrip_);
    addAndMakeVisible(patternSelector_);
    setWantsKeyboardFocus(true);

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
    playButton_.setClickingTogglesState(true);
    playButton_.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF2A2A2Au));
    playButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF27AE60u));
    playButton_.onClick = [this] {
        bool playing = playButton_.getToggleState();
        processorRef_.setStandalonePlaying(playing);
        playButton_.setButtonText(playing ? "STOP" : "PLAY");
    };
    playButton_.setVisible(standalone);

    setSize(900, 600);
    startTimerHz(10); // sync play button state with clock
}

LayerzEditor::~LayerzEditor() { stopTimer(); }

bool LayerzEditor::keyPressed(const juce::KeyPress& key) {
    bool cmd = key.getModifiers().isCommandDown();
    if (cmd && key.getKeyCode() == 'S') {
        auto fc = std::make_shared<juce::FileChooser>("Save project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.layerz");
        fc->launchAsync(juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles,
            [this, fc](const juce::FileChooser&) mutable {
                auto f = fc->getResult();
                if (f.getFullPathName().isNotEmpty())
                    processorRef_.projectStore().saveToFile(f.withFileExtension("layerz"));
            });
        return true;
    }
    if (cmd && key.getKeyCode() == 'O') {
        auto fc = std::make_shared<juce::FileChooser>("Open project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.layerz");
        fc->launchAsync(juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles,
            [this, fc](const juce::FileChooser&) mutable {
                auto f = fc->getResult();
                if (f.existsAsFile())
                    processorRef_.projectStore().loadFromFile(f);
            });
        return true;
    }
    return false;
}

void LayerzEditor::timerCallback() {
    if (! processorRef_.getClock().isStandaloneMode()) return;
    bool playing = processorRef_.getClock().isPlaying();
    // Sync button toggle state without firing onClick
    if (playButton_.getToggleState() != playing) {
        playButton_.setToggleState(playing, juce::dontSendNotification);
        playButton_.setButtonText(playing ? "STOP" : "PLAY");
    }
}

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
    // Pattern selector always visible — right-aligned in top bar
    patternSelector_.setBounds(getWidth() - 120, 4, 116, kTopBarHeight - 8);
    beatStrip_.setBounds(0, y, getWidth(), kBeatStripH);
    bassStrip_.setBounds(0, y + kBeatStripH, getWidth(), 80);  // 24px header + 56px row
}
