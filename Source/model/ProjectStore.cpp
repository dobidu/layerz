#include "ProjectStore.h"
#include <juce_core/juce_core.h>

// JSON helpers using JUCE's var/JSON — no external dependency needed for F0.
// nlohmann/json will be added in F1 when schema grows. JUCE var is sufficient
// for the F0 round-trip test (one pattern, two layers, four events).

static juce::String profileToString(Profile p) {
    if (p == Profile::STANDALONE) return "STANDALONE";
    if (p == Profile::AUTO)       return "AUTO";
    return "PLUGIN";
}

static Profile profileFromString(const juce::String& s) {
    if (s == "STANDALONE") return Profile::STANDALONE;
    if (s == "AUTO")        return Profile::AUTO;
    return Profile::PLUGIN;
}

static juce::String layerTypeToString(LayerType t) {
    switch (t) {
        case LayerType::BEAT:     return "BEAT";
        case LayerType::BASS:     return "BASS";
        case LayerType::HARMONIC: return "HARMONIC";
        case LayerType::MELODIC:  return "MELODIC";
    }
    return "BEAT";
}

static LayerType layerTypeFromString(const juce::String& s) {
    if (s == "BASS")     return LayerType::BASS;
    if (s == "HARMONIC") return LayerType::HARMONIC;
    if (s == "MELODIC")  return LayerType::MELODIC;
    return LayerType::BEAT;
}

// ── Serialise ────────────────────────────────────────────────────────────────

static juce::var eventToVar(const Event& e) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("step",               e.step);
    obj->setProperty("velocity",           e.velocity);
    obj->setProperty("micro_offset_ticks", e.micro_offset_ticks);
    obj->setProperty("midi_note",          e.midi_note);
    return obj;
}

static juce::String waveformToString(Waveform w) {
    return w == Waveform::SQUARE ? "SQUARE" : "SAW";
}
static Waveform waveformFromString(const juce::String& s) {
    return s == "SQUARE" ? Waveform::SQUARE : Waveform::SAW;
}

static juce::var bassVoiceParamsToVar(const BassVoiceParams& b) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("waveform",          waveformToString(b.waveform));
    obj->setProperty("filter_cutoff",     b.filter_cutoff);
    obj->setProperty("filter_resonance",  b.filter_resonance);
    obj->setProperty("env_attack_ms",     b.env_attack_ms);
    obj->setProperty("env_decay_ms",      b.env_decay_ms);
    obj->setProperty("env_sustain",       b.env_sustain);
    obj->setProperty("env_release_ms",    b.env_release_ms);
    obj->setProperty("glide_ms",          b.glide_ms);
    obj->setProperty("volume",            b.volume);
    return obj;
}
static juce::Result varToBassVoiceParams(const juce::var& v, BassVoiceParams& out) {
    if (! v.isObject()) return juce::Result::ok(); // missing → defaults
    out.waveform         = waveformFromString(v["waveform"].toString());
    out.filter_cutoff    = static_cast<float>(static_cast<double>(v["filter_cutoff"]));
    out.filter_resonance = static_cast<float>(static_cast<double>(v["filter_resonance"]));
    out.env_attack_ms    = static_cast<float>(static_cast<double>(v["env_attack_ms"]));
    out.env_decay_ms     = static_cast<float>(static_cast<double>(v["env_decay_ms"]));
    out.env_sustain      = static_cast<float>(static_cast<double>(v["env_sustain"]));
    out.env_release_ms   = static_cast<float>(static_cast<double>(v["env_release_ms"]));
    out.glide_ms         = static_cast<float>(static_cast<double>(v["glide_ms"]));
    out.volume           = static_cast<float>(static_cast<double>(v["volume"]));
    return juce::Result::ok();
}

static juce::var patternChainEntryToVar(const PatternChainEntry& e) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("pattern_id", juce::String(e.pattern_id));
    obj->setProperty("bars",       e.bars);
    return obj;
}
static juce::Result varToPatternChainEntry(const juce::var& v, PatternChainEntry& out) {
    if (! v.isObject()) return juce::Result::fail("PatternChainEntry is not an object");
    out.pattern_id = v["pattern_id"].toString().toStdString();
    out.bars       = static_cast<int>(v["bars"]);
    return juce::Result::ok();
}

static juce::var aestheticsToVar(const Aesthetics& a) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("drag",          a.drag);
    obj->setProperty("push",          a.push);
    obj->setProperty("roll_mult",      a.roll_mult);
    obj->setProperty("roll_vel_decay", a.roll_vel_decay);
    obj->setProperty("stutter_reps",   a.stutter_reps);
    obj->setProperty("stutter_gate",   a.stutter_gate);
    obj->setProperty("fracture_prob",  a.fracture_prob);
    // fracture_seed: store as int64 to avoid uint32 sign ambiguity in juce::var
    obj->setProperty("fracture_seed",  static_cast<juce::int64>(a.fracture_seed));
    return obj;
}

static juce::var drumTrackToVar(const DrumTrack& dt) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("voice_type", juce::String(dt.voice_type));
    obj->setProperty("level",      dt.level);
    obj->setProperty("mute",       dt.mute);
    obj->setProperty("param1",     dt.param1);
    juce::Array<juce::var> evs;
    for (const auto& e : dt.events) evs.add(eventToVar(e));
    obj->setProperty("events", evs);
    return obj;
}

static juce::var layerToVar(const Layer& l) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("type",      layerTypeToString(l.type));
    obj->setProperty("voice_ref", juce::String(l.voice_ref));
    obj->setProperty("aesthetics", aestheticsToVar(l.aesthetics));
    juce::Array<juce::var> evs;
    for (const auto& e : l.events) evs.add(eventToVar(e));
    obj->setProperty("events", evs);
    juce::Array<juce::var> dts;
    for (const auto& dt : l.drum_tracks) dts.add(drumTrackToVar(dt));
    obj->setProperty("drum_tracks",    dts);
    obj->setProperty("bass_params",    bassVoiceParamsToVar(l.bass_params));
    obj->setProperty("template_name",  juce::String(l.template_name));
    obj->setProperty("morph_amount",   l.morph_amount);
    return obj;
}

static juce::var patternToVar(const Pattern& p) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id",           juce::String(p.id));
    obj->setProperty("length_steps", p.length_steps);
    juce::Array<juce::var> layers;
    for (const auto& l : p.layers) layers.add(layerToVar(l));
    obj->setProperty("layers", layers);
    return obj;
}

juce::String ProjectStore::toJson(const Project& p) {
    auto* root = new juce::DynamicObject();
    root->setProperty("schema_version", p.schema_version);
    root->setProperty("tempo",          p.tempo);
    root->setProperty("swing_global",   p.swing_global);
    root->setProperty("profile",        profileToString(p.profile));
    juce::Array<juce::var> patterns;
    for (const auto& pat : p.patterns) patterns.add(patternToVar(pat));
    root->setProperty("patterns", patterns);
    root->setProperty("active_pattern_index", p.active_pattern_index);
    juce::Array<juce::var> chain;
    for (const auto& e : p.chain) chain.add(patternChainEntryToVar(e));
    root->setProperty("chain", chain);
    return juce::JSON::toString(juce::var(root), false);
}

// ── Deserialise ──────────────────────────────────────────────────────────────

static juce::Result varToEvent(const juce::var& v, Event& out) {
    if (! v.isObject()) return juce::Result::fail("Event is not an object");
    out.step               = static_cast<int>  (v["step"]);
    out.velocity           = static_cast<float>(static_cast<double>(v["velocity"]));
    out.micro_offset_ticks = static_cast<int>  (v["micro_offset_ticks"]);
    out.midi_note          = static_cast<int>  (v["midi_note"]);
    return juce::Result::ok();
}

static juce::Result varToAesthetics(const juce::var& v, Aesthetics& out) {
    if (! v.isObject()) return juce::Result::fail("Aesthetics is not an object");
    out.drag          = static_cast<float>(static_cast<double>(v["drag"]));
    out.push          = static_cast<float>(static_cast<double>(v["push"]));
    out.roll_mult      = static_cast<int>(v["roll_mult"]);
    out.roll_vel_decay = static_cast<float>(static_cast<double>(v["roll_vel_decay"]));
    out.stutter_reps   = static_cast<int>(v["stutter_reps"]);
    out.stutter_gate   = static_cast<float>(static_cast<double>(v["stutter_gate"]));
    out.fracture_prob  = static_cast<float>(static_cast<double>(v["fracture_prob"]));
    out.fracture_seed  = static_cast<uint32_t>(static_cast<juce::int64>(v["fracture_seed"]));
    return juce::Result::ok();
}

static juce::Result varToDrumTrack(const juce::var& v, DrumTrack& out) {
    if (! v.isObject()) return juce::Result::fail("DrumTrack is not an object");
    out.voice_type = v["voice_type"].toString().toStdString();
    out.level      = static_cast<float>(static_cast<double>(v["level"]));
    out.mute       = static_cast<bool>(v["mute"]);
    out.param1     = static_cast<float>(static_cast<double>(v["param1"]));
    if (const auto* evArr = v["events"].getArray()) {
        for (const auto& ev : *evArr) {
            Event e;
            if (auto r = varToEvent(ev, e); r.failed()) return r;
            out.events.push_back(e);
        }
    }
    return juce::Result::ok();
}

static juce::Result varToLayer(const juce::var& v, Layer& out) {
    if (! v.isObject()) return juce::Result::fail("Layer is not an object");
    out.type      = layerTypeFromString(v["type"].toString());
    out.voice_ref = v["voice_ref"].toString().toStdString();
    if (auto r = varToAesthetics(v["aesthetics"], out.aesthetics); r.failed()) return r;
    if (const auto* evArr = v["events"].getArray()) {
        for (const auto& ev : *evArr) {
            Event e;
            if (auto r = varToEvent(ev, e); r.failed()) return r;
            out.events.push_back(e);
        }
    }
    // Missing drum_tracks field → empty vector → safe (backward compat with F0 files)
    if (const auto* dtArr = v["drum_tracks"].getArray()) {
        for (const auto& dt : *dtArr) {
            DrumTrack track;
            if (auto r = varToDrumTrack(dt, track); r.failed()) return r;
            out.drum_tracks.push_back(track);
        }
    }
    // Missing bass_params → default-initialised (backward compat)
    if (v["bass_params"].isObject())
        if (auto r = varToBassVoiceParams(v["bass_params"], out.bass_params); r.failed()) return r;
    // template_name + morph_amount — missing in older files → safe defaults
    out.template_name = v["template_name"].toString().toStdString();
    out.morph_amount  = static_cast<float>(static_cast<double>(v["morph_amount"]));
    return juce::Result::ok();
}

static juce::Result varToPattern(const juce::var& v, Pattern& out) {
    if (! v.isObject()) return juce::Result::fail("Pattern is not an object");
    out.id           = v["id"].toString().toStdString();
    out.length_steps = static_cast<int>(v["length_steps"]);
    if (const auto* lArr = v["layers"].getArray()) {
        for (const auto& l : *lArr) {
            Layer layer;
            if (auto r = varToLayer(l, layer); r.failed()) return r;
            out.layers.push_back(layer);
        }
    }
    return juce::Result::ok();
}

juce::Result ProjectStore::fromJson(const juce::String& json, Project& out) {
    juce::var parsed;
    auto parseResult = juce::JSON::parse(json, parsed);
    if (parseResult.failed())
        return juce::Result::fail("JSON parse error: " + parseResult.getErrorMessage());
    if (! parsed.isObject())
        return juce::Result::fail("Root is not a JSON object");

    // Schema version check: unknown future versions are rejected, not silently loaded
    int version = static_cast<int>(parsed["schema_version"]);
    if (version < 1 || version > CURRENT_SCHEMA_VERSION)
        return juce::Result::fail("Unsupported schema_version: " + juce::String(version));

    out.schema_version = version;
    out.tempo          = static_cast<double>(parsed["tempo"]);
    out.swing_global   = static_cast<float> (static_cast<double>(parsed["swing_global"]));
    out.profile        = profileFromString(parsed["profile"].toString());

    if (const auto* pArr = parsed["patterns"].getArray()) {
        for (const auto& p : *pArr) {
            Pattern pat;
            if (auto r = varToPattern(p, pat); r.failed()) return r;
            out.patterns.push_back(pat);
        }
    }
    // chain and active_pattern_index — missing in older files → safe defaults
    out.active_pattern_index = static_cast<int>(parsed["active_pattern_index"]);
    if (const auto* cArr = parsed["chain"].getArray()) {
        for (const auto& e : *cArr) {
            PatternChainEntry entry;
            if (auto r = varToPatternChainEntry(e, entry); r.failed()) return r;
            out.chain.push_back(entry);
        }
    }
    return juce::Result::ok();
}

juce::Result ProjectStore::saveToFile(const juce::File& target) const {
    auto snap = snapshot();
    auto json = toJson(*snap);
    auto tmp  = target.getSiblingFile(target.getFileName() + ".tmp");
    if (! tmp.replaceWithText(json))
        return juce::Result::fail("Could not write to " + tmp.getFullPathName());
    if (! tmp.moveFileTo(target))
        return juce::Result::fail("Could not rename tmp to " + target.getFullPathName());
    return juce::Result::ok();
}

juce::Result ProjectStore::loadFromFile(const juce::File& source) {
    if (! source.existsAsFile())
        return juce::Result::fail("File not found: " + source.getFullPathName());
    auto json = source.loadFileAsString();
    if (json.isEmpty())
        return juce::Result::fail("File is empty: " + source.getFullPathName());
    Project loaded;
    if (auto r = fromJson(json, loaded); r.failed()) return r;
    postMutation([&loaded](Project& p) { p = loaded; });
    return juce::Result::ok();
}
