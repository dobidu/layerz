#include "PitchedStepRow.h"
#include <algorithm>

static const juce::Colour kAmber (0xFFE67E22u);
static const juce::Colour kDark  (0xFF2A2A2Au);

// File-scope so lambdas can copy values directly
static const int kBassNotes[] = {
    36,38,40,41,43,45,46,47,  // C2–B2
    48,50,52,53,55,57,59,60,  // C3–C4
};

PitchedStepRow::PitchedStepRow(ProjectStore& store) : store_(store) {
    for (int i = 0; i < 16; ++i) {
        addAndMakeVisible(stepButtons_[i]);
        stepButtons_[i].setColour(juce::TextButton::buttonColourId, kDark);

        stepButtons_[i].onClick = [this, i] () {
            auto snap = store_.snapshot();
            bool wasActive = hasEventAtStep(*snap, i);

            if (wasActive) {
                // Optimistic clear
                stepButtons_[i].setButtonText("");
                stepButtons_[i].setColour(juce::TextButton::buttonColourId, kDark);
                store_.postMutation([i](Project& p) {
                    if (p.patterns.empty()) return;
                    int pi = juce::jlimit(0,(int)p.patterns.size()-1, p.active_pattern_index);
                    for (auto& l : p.patterns[static_cast<std::size_t>(pi)].layers) {
                        if (l.type != LayerType::BASS) continue;
                        l.events.erase(std::remove_if(l.events.begin(), l.events.end(),
                            [i](const Event& e){ return e.step == i; }), l.events.end());
                        break;
                    }
                });
            } else {
                // Show note picker popup
                juce::PopupMenu m;
                for (int ni = 0; ni < 16; ++ni) {
                    int note = kBassNotes[ni];
                    m.addItem(ni + 1, noteName(note) + " (" + juce::String(note) + ")");
                }
                m.showMenuAsync(juce::PopupMenu::Options{}, [this, i](int result) {
                    if (result < 1) return;
                    int midiNote = kBassNotes[result - 1];
                    // Optimistic update
                    stepButtons_[i].setButtonText(noteName(midiNote));
                    stepButtons_[i].setColour(juce::TextButton::buttonColourId, kAmber);
                    store_.postMutation([i, midiNote](Project& p) {
                        if (p.patterns.empty()) return;
                        int pi = juce::jlimit(0,(int)p.patterns.size()-1, p.active_pattern_index);
                        for (auto& l : p.patterns[static_cast<std::size_t>(pi)].layers) {
                            if (l.type != LayerType::BASS) continue;
                            l.events.erase(std::remove_if(l.events.begin(), l.events.end(),
                                [i](const Event& e){ return e.step == i; }), l.events.end());
                            Event e; e.step = i; e.midi_note = midiNote; e.velocity = 0.8f;
                            l.events.push_back(e);
                            break;
                        }
                    });
                });
            }
        };
    }
}

const Layer* PitchedStepRow::findBassLayer(const Project& snap) const {
    if (snap.patterns.empty()) return nullptr;
    int pi = juce::jlimit(0,(int)snap.patterns.size()-1, snap.active_pattern_index);
    for (const auto& l : snap.patterns[static_cast<std::size_t>(pi)].layers)
        if (l.type == LayerType::BASS) return &l;
    return nullptr;
}

bool PitchedStepRow::hasEventAtStep(const Project& snap, int step) const {
    const auto* l = findBassLayer(snap);
    if (! l) return false;
    return std::any_of(l->events.begin(), l->events.end(),
        [step](const Event& e){ return e.step == step; });
}

int PitchedStepRow::noteAtStep(const Project& snap, int step) const {
    const auto* l = findBassLayer(snap);
    if (! l) return -1;
    for (const auto& e : l->events)
        if (e.step == step) return e.midi_note;
    return -1;
}

void PitchedStepRow::setCurrentStep(int step) noexcept {
    if (currentStep_ != step) { currentStep_ = step; repaint(); }
}

void PitchedStepRow::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF1E1E1Eu));
    // Step indicator bar
    if (currentStep_ >= 0 && currentStep_ < 16) {
        int w = getWidth() / 16;
        g.setColour(kAmber.withAlpha(0.9f));
        g.fillRect(currentStep_ * w, getHeight() - 3, w, 3);
    }
}

void PitchedStepRow::resized() {
    int w = getWidth() / 16;
    for (int i = 0; i < 16; ++i)
        stepButtons_[i].setBounds(i * w, 0, w, getHeight() - 4);
}

void PitchedStepRow::updateFromSnapshot(const Project& snap) {
    for (int i = 0; i < 16; ++i) {
        int note = noteAtStep(snap, i);
        bool active = (note >= 0);
        stepButtons_[i].setColour(juce::TextButton::buttonColourId,
                                  active ? kAmber : kDark);
        stepButtons_[i].setButtonText(active ? noteName(note) : "");
        stepButtons_[i].repaint();
    }
}
