#pragma once
#include <string>
#include <vector>
#include <cmath>

// Minimal v0 schema — enough for F0 round-trip test.
// Expand field by field as each phase adds data (F1-F4).
// No JUCE dependencies: includable in LLM bridge and bench tools without JUCE.

static constexpr int CURRENT_SCHEMA_VERSION = 1;

struct Event {
    int   step                = 0;
    float velocity            = 1.0f;
    int   micro_offset_ticks  = 0;
    // pitch, length, params added in F1-F4
};

struct Aesthetics {
    float drag = 0.0f;
    float push = 0.0f;
    // roll, stutter, fracture added in F3
};

struct DrumTrack {
    std::string       voice_type;        // "kick" | "snare" | "hat" | "perc"
    float             level  = 1.0f;
    bool              mute   = false;
    float             param1 = 100.0f;   // kick=pitch_decay_ms, snare=noise_decay_ms,
                                         // hat=decay_ms, perc=freq_hz
    std::vector<Event> events;
};

enum class LayerType { BEAT, BASS, HARMONIC, MELODIC };

struct Layer {
    LayerType        type  = LayerType::BEAT;
    std::string      voice_ref;
    std::vector<Event> events;           // non-BEAT layers (F2+)
    std::vector<DrumTrack> drum_tracks;  // BEAT layer: 4 tracks (kick/snare/hat/perc)
    Aesthetics       aesthetics;
};

struct Pattern {
    std::string        id;
    int                length_steps = 16;
    std::vector<Layer> layers;
};

enum class Profile { PLUGIN, STANDALONE, AUTO };

struct Project {
    int schema_version = CURRENT_SCHEMA_VERSION;  // migration sentinel — never remove
    double tempo        = 120.0;
    float  swing_global = 0.0f;
    Profile profile     = Profile::AUTO;
    std::vector<Pattern> patterns;
    // voices[], modulations[], scenes[] added in later phases
};

// Equality helpers — float fields use epsilon comparison to survive JSON round-trip
inline bool operator==(const Event& a, const Event& b) {
    return a.step               == b.step
        && a.micro_offset_ticks == b.micro_offset_ticks
        && std::abs(a.velocity  -  b.velocity) < 1e-6f;
}
inline bool operator==(const Aesthetics& a, const Aesthetics& b) {
    return std::abs(a.drag - b.drag) < 1e-6f
        && std::abs(a.push - b.push) < 1e-6f;
}
inline bool operator==(const DrumTrack& a, const DrumTrack& b) {
    return a.voice_type == b.voice_type
        && std::abs(a.level  - b.level)  < 1e-6f
        && a.mute == b.mute
        && std::abs(a.param1 - b.param1) < 1e-6f
        && a.events == b.events;
}
inline bool operator==(const Layer& a, const Layer& b) {
    return a.type == b.type && a.voice_ref == b.voice_ref
        && a.events == b.events && a.drum_tracks == b.drum_tracks
        && a.aesthetics == b.aesthetics;
}
inline bool operator==(const Pattern& a, const Pattern& b) {
    return a.id == b.id && a.length_steps == b.length_steps && a.layers == b.layers;
}
inline bool operator==(const Project& a, const Project& b) {
    return a.schema_version == b.schema_version
        && std::abs(a.tempo        - b.tempo)        < 1e-9
        && std::abs(a.swing_global - b.swing_global) < 1e-6f
        && a.profile  == b.profile
        && a.patterns == b.patterns;
}
