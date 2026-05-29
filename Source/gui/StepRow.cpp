#include "StepRow.h"
#include <algorithm>

static const juce::Colour kCrimson  (0xFFC0392Bu);
static const juce::Colour kDarkGrey (0xFF2A2A2Au);
static const juce::Colour kSeparator(0xFF333333u);

StepRow::StepRow(ProjectStore& store, int trackIndex)
    : store_(store), trackIndex_(trackIndex)
{
    for (int i = 0; i < 16; ++i) {
        addAndMakeVisible(stepButtons_[i]);
        stepButtons_[i].setColour(juce::TextButton::buttonColourId, kDarkGrey);
        stepButtons_[i].setColour(juce::TextButton::buttonOnColourId, kCrimson);

        stepButtons_[i].onClick = [this, i] {
            auto snap = store_.snapshot();
            bool wasActive = hasEventAtStep(*snap, i);

            // Optimistic visual update — immediate feedback before postMutation confirms
            stepButtons_[i].setToggleState(!wasActive, juce::dontSendNotification);
            stepButtons_[i].setColour(juce::TextButton::buttonColourId,
                                      wasActive ? kDarkGrey : kCrimson);

            store_.postMutation([this, i, wasActive](Project& p) {
                if (p.patterns.empty() || p.patterns[0].layers.empty()) return;
                auto& tracks = p.patterns[0].layers[0].drum_tracks;
                if (trackIndex_ >= static_cast<int>(tracks.size())) return;
                auto& evts = tracks[static_cast<std::size_t>(trackIndex_)].events;

                if (wasActive) {
                    // Remove ALL events at this step (audit M8: std::remove_if)
                    evts.erase(std::remove_if(evts.begin(), evts.end(),
                        [i](const Event& e){ return e.step == i; }), evts.end());
                } else {
                    Event e; e.step = i; e.velocity = 0.8f; e.micro_offset_ticks = 0;
                    evts.push_back(e);
                }
            });
        };
    }
}

bool StepRow::hasEventAtStep(const Project& snap, int step) const {
    if (snap.patterns.empty() || snap.patterns[0].layers.empty()) return false;
    const auto& tracks = snap.patterns[0].layers[0].drum_tracks;
    if (trackIndex_ >= static_cast<int>(tracks.size())) return false;
    const auto& evts = tracks[static_cast<std::size_t>(trackIndex_)].events;
    return std::any_of(evts.begin(), evts.end(),
                       [step](const Event& e){ return e.step == step; });
}

void StepRow::setCurrentStep(int step) noexcept {
    if (currentStep_ != step) {
        currentStep_ = step;
        repaint();
    }
}

void StepRow::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF1E1E1Eu));
    // Draw playhead indicator below the current step button
    if (currentStep_ >= 0 && currentStep_ < 16) {
        int w = getWidth() / 16;
        int x = currentStep_ * w;
        g.setColour(juce::Colour(0xFFFFFFFFu).withAlpha(0.9f));
        g.fillRect(x, getHeight() - 3, w, 3);
    }
}

void StepRow::resized() {
    auto bounds = getLocalBounds();
    int w = bounds.getWidth() / 16;
    for (int i = 0; i < 16; ++i)
        stepButtons_[i].setBounds(bounds.getX() + i * w, bounds.getY(), w, bounds.getHeight() - 4);
}

void StepRow::updateFromSnapshot(const Project& snap) {
    for (int i = 0; i < 16; ++i) {
        bool active = hasEventAtStep(snap, i);
        // dontSendNotification prevents onClick callbacks during timer update (audit M7)
        stepButtons_[i].setToggleState(active, juce::dontSendNotification);
        stepButtons_[i].setColour(juce::TextButton::buttonColourId,
                                  active ? kCrimson : kDarkGrey);
        stepButtons_[i].repaint();
    }
}
