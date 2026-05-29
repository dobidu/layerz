#include "VoiceParamPanel.h"

static const char* kVoiceNames[]  = { "KICK", "SNARE", "HAT", "PERC" };
static const char* kParam1Labels[]= { "DECAY", "SNAP", "DECAY", "FREQ" };
static const float kParam1Min[]   = { 20.0f, 30.0f, 10.0f, 60.0f };
static const float kParam1Max[]   = { 500.0f, 200.0f, 80.0f, 800.0f };

const char* VoiceParamPanel::voiceName(int idx) noexcept  { return kVoiceNames[idx & 3]; }
const char* VoiceParamPanel::param1Label(int idx) noexcept{ return kParam1Labels[idx & 3]; }
float VoiceParamPanel::param1Min(int idx) noexcept { return kParam1Min[idx & 3]; }
float VoiceParamPanel::param1Max(int idx) noexcept { return kParam1Max[idx & 3]; }

VoiceParamPanel::VoiceParamPanel(ProjectStore& store, int trackIndex)
    : store_(store), trackIndex_(trackIndex)
{
    addAndMakeVisible(voiceLabel_);
    voiceLabel_.setText(voiceName(trackIndex_), juce::dontSendNotification);
    voiceLabel_.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    voiceLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    voiceLabel_.setJustificationType(juce::Justification::centred);

    // Level slider
    addAndMakeVisible(levelSlider_);
    levelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    levelSlider_.setRange(0.0, 1.0, 0.0);
    levelSlider_.setValue(1.0, juce::dontSendNotification);
    levelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    levelSlider_.onValueChange = [this] {
        float newVal = static_cast<float>(levelSlider_.getValue());
        store_.postMutation([this, newVal](Project& p) {
            if (p.patterns.empty() || p.patterns[0].layers.empty()) return;
            auto& tracks = p.patterns[0].layers[0].drum_tracks;
            if (trackIndex_ < static_cast<int>(tracks.size()))
                tracks[static_cast<std::size_t>(trackIndex_)].level = newVal;
        });
    };

    // Mute button
    addAndMakeVisible(muteButton_);
    muteButton_.setButtonText("M");
    muteButton_.setToggleable(true);
    muteButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2Au));
    muteButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFE74C3Cu));
    muteButton_.onClick = [this] {
        bool muted = muteButton_.getToggleState();
        store_.postMutation([this, muted](Project& p) {
            if (p.patterns.empty() || p.patterns[0].layers.empty()) return;
            auto& tracks = p.patterns[0].layers[0].drum_tracks;
            if (trackIndex_ < static_cast<int>(tracks.size()))
                tracks[static_cast<std::size_t>(trackIndex_)].mute = muted;
        });
    };

    // Param1 slider
    addAndMakeVisible(param1Slider_);
    param1Slider_.setSliderStyle(juce::Slider::LinearHorizontal);
    param1Slider_.setRange(param1Min(trackIndex_), param1Max(trackIndex_), 0.0);
    param1Slider_.setValue(100.0, juce::dontSendNotification);
    param1Slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    param1Slider_.onValueChange = [this] {
        float newVal = static_cast<float>(param1Slider_.getValue());
        store_.postMutation([this, newVal](Project& p) {
            if (p.patterns.empty() || p.patterns[0].layers.empty()) return;
            auto& tracks = p.patterns[0].layers[0].drum_tracks;
            if (trackIndex_ < static_cast<int>(tracks.size()))
                tracks[static_cast<std::size_t>(trackIndex_)].param1 = newVal;
        });
    };
}

void VoiceParamPanel::resized() {
    auto b = getLocalBounds();
    int h = b.getHeight();
    int quarter = h / 4;
    voiceLabel_ .setBounds(b.getX(), b.getY(),           b.getWidth(), quarter);
    levelSlider_.setBounds(b.getX(), b.getY() + quarter, b.getWidth(), quarter);
    muteButton_ .setBounds(b.getX(), b.getY() + quarter * 2, b.getWidth(), quarter);
    param1Slider_.setBounds(b.getX(), b.getY() + quarter * 3, b.getWidth(), quarter);
}

void VoiceParamPanel::updateFromSnapshot(const Project& snap) {
    if (snap.patterns.empty() || snap.patterns[0].layers.empty()) return;
    const auto& tracks = snap.patterns[0].layers[0].drum_tracks;
    if (trackIndex_ >= static_cast<int>(tracks.size())) return;
    const auto& track = tracks[static_cast<std::size_t>(trackIndex_)];

    // dontSendNotification prevents onChange callbacks during timer update (audit M6)
    levelSlider_.setValue(static_cast<double>(track.level),  juce::dontSendNotification);
    param1Slider_.setValue(static_cast<double>(track.param1), juce::dontSendNotification);
    muteButton_.setToggleState(track.mute, juce::dontSendNotification);
}
