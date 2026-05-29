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
    return obj;
}

static juce::var aestheticsToVar(const Aesthetics& a) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("drag", a.drag);
    obj->setProperty("push", a.push);
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
    return juce::JSON::toString(juce::var(root), false);
}

// ── Deserialise ──────────────────────────────────────────────────────────────

static juce::Result varToEvent(const juce::var& v, Event& out) {
    if (! v.isObject()) return juce::Result::fail("Event is not an object");
    out.step               = static_cast<int>  (v["step"]);
    out.velocity           = static_cast<float>(static_cast<double>(v["velocity"]));
    out.micro_offset_ticks = static_cast<int>  (v["micro_offset_ticks"]);
    return juce::Result::ok();
}

static juce::Result varToAesthetics(const juce::var& v, Aesthetics& out) {
    if (! v.isObject()) return juce::Result::fail("Aesthetics is not an object");
    out.drag = static_cast<float>(static_cast<double>(v["drag"]));
    out.push = static_cast<float>(static_cast<double>(v["push"]));
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
