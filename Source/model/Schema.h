#pragma once
#include <string>
#include <vector>
#include <cmath>

// No JUCE dependencies: includable in LLM bridge and bench tools without JUCE.

static constexpr int CURRENT_SCHEMA_VERSION = 1;

struct Event {
    int   step                = 0;
    float velocity            = 1.0f;
    int   micro_offset_ticks  = 0;
    int   midi_note           = -1;  // -1 = pitch-less (BEAT). 0-127 = MIDI note.
};

struct Aesthetics {
    // DRAG / PUSH — micro-timing offsets
    float drag  = 0.0f;  // 0-1: fraction of step period to lag behind grid
    float push  = 0.0f;  // 0-1: fraction of step period to push ahead of grid
    // ROLL — step density multiplier
    int   roll_mult      = 1;     // 1=off, 2/3/4/8 = subdivisions per step
    float roll_vel_decay = 0.5f;  // velocity multiplier per sub-hit (0-1)
    // STUTTER — gated short repetitions
    int   stutter_reps   = 0;     // 0=off, 1-8 repetitions
    float stutter_gate   = 0.25f; // gate time as fraction of step period (0-1)
    // FRACTURE — probabilistic firing
    float    fracture_prob = 0.0f;   // 0-1: per-step probability to fire
    uint32_t fracture_seed = 12345u; // LCG seed; reset at pattern loop start
};

struct DrumTrack {
    std::string       voice_type;
    float             level  = 1.0f;
    bool              mute   = false;
    float             param1 = 100.0f;
    std::vector<Event> events;
};

enum class Waveform { SAW, SQUARE };

struct BassVoiceParams {
    Waveform waveform          = Waveform::SAW;
    float    filter_cutoff     = 800.0f;   // Hz, 80–8000
    float    filter_resonance  = 0.3f;     // 0–1
    float    env_attack_ms     = 5.0f;     // 1–500ms
    float    env_decay_ms      = 100.0f;   // 1–500ms
    float    env_sustain       = 0.8f;     // 0–1
    float    env_release_ms    = 80.0f;    // 1–500ms
    float    glide_ms          = 0.0f;     // 0 = no glide
    float    volume            = 1.0f;
};

struct PatternChainEntry {
    std::string pattern_id;
    int bars = 1;  // bars this pattern plays before advancing
};

enum class LayerType { BEAT, BASS, HARMONIC, MELODIC };

struct Layer {
    LayerType           type  = LayerType::BEAT;
    std::string         voice_ref;
    std::vector<Event>  events;           // non-BEAT layers (F2+)
    std::vector<DrumTrack> drum_tracks;   // BEAT layer
    BassVoiceParams     bass_params;      // BASS layer
    Aesthetics          aesthetics;
    std::string         template_name;    // "" = no template; F3 template presets
    float               morph_amount = 0.0f; // 0=neutral, 1=full template
};

struct Pattern {
    std::string        id;
    int                length_steps = 16;
    std::vector<Layer> layers;
};

enum class Profile { PLUGIN, STANDALONE, AUTO };

struct Project {
    int schema_version     = CURRENT_SCHEMA_VERSION;
    double tempo           = 120.0;
    float  swing_global    = 0.0f;
    Profile profile        = Profile::AUTO;
    std::vector<Pattern>           patterns;
    std::vector<PatternChainEntry> chain;               // ordered playback sequence
    int active_pattern_index = 0;                       // edit-view pattern index
};

// Equality helpers — float fields use epsilon comparison to survive JSON round-trip
inline bool operator==(const Event& a, const Event& b) {
    return a.step               == b.step
        && a.micro_offset_ticks == b.micro_offset_ticks
        && a.midi_note          == b.midi_note
        && std::abs(a.velocity  -  b.velocity) < 1e-6f;
}
inline bool operator==(const Aesthetics& a, const Aesthetics& b) {
    return std::abs(a.drag         - b.drag)         < 1e-6f
        && std::abs(a.push         - b.push)         < 1e-6f
        && a.roll_mult             == b.roll_mult
        && std::abs(a.roll_vel_decay - b.roll_vel_decay) < 1e-6f
        && a.stutter_reps          == b.stutter_reps
        && std::abs(a.stutter_gate - b.stutter_gate) < 1e-6f
        && std::abs(a.fracture_prob - b.fracture_prob) < 1e-6f
        && a.fracture_seed         == b.fracture_seed;
}
inline bool operator==(const DrumTrack& a, const DrumTrack& b) {
    return a.voice_type == b.voice_type
        && std::abs(a.level  - b.level)  < 1e-6f
        && a.mute == b.mute
        && std::abs(a.param1 - b.param1) < 1e-6f
        && a.events == b.events;
}
inline bool operator==(const BassVoiceParams& a, const BassVoiceParams& b) {
    return a.waveform == b.waveform
        && std::abs(a.filter_cutoff    - b.filter_cutoff)    < 1e-6f
        && std::abs(a.filter_resonance - b.filter_resonance) < 1e-6f
        && std::abs(a.env_attack_ms    - b.env_attack_ms)    < 1e-6f
        && std::abs(a.env_decay_ms     - b.env_decay_ms)     < 1e-6f
        && std::abs(a.env_sustain      - b.env_sustain)      < 1e-6f
        && std::abs(a.env_release_ms   - b.env_release_ms)   < 1e-6f
        && std::abs(a.glide_ms         - b.glide_ms)         < 1e-6f
        && std::abs(a.volume           - b.volume)           < 1e-6f;
}
inline bool operator==(const PatternChainEntry& a, const PatternChainEntry& b) {
    return a.pattern_id == b.pattern_id && a.bars == b.bars;
}
inline bool operator==(const Layer& a, const Layer& b) {
    return a.type == b.type && a.voice_ref == b.voice_ref
        && a.events == b.events && a.drum_tracks == b.drum_tracks
        && a.bass_params == b.bass_params && a.aesthetics == b.aesthetics
        && a.template_name == b.template_name
        && std::abs(a.morph_amount - b.morph_amount) < 1e-6f;
}
inline bool operator==(const Pattern& a, const Pattern& b) {
    return a.id == b.id && a.length_steps == b.length_steps && a.layers == b.layers;
}
inline bool operator==(const Project& a, const Project& b) {
    return a.schema_version      == b.schema_version
        && std::abs(a.tempo        - b.tempo)        < 1e-9
        && std::abs(a.swing_global - b.swing_global) < 1e-6f
        && a.profile               == b.profile
        && a.patterns              == b.patterns
        && a.chain                 == b.chain
        && a.active_pattern_index  == b.active_pattern_index;
}
