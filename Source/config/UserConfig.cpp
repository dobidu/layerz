#include "UserConfig.h"

juce::File UserConfig::configFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("LAYERZ")
               .getChildFile("config.json");
}

UserConfig UserConfig::load() {
    auto f = configFile();
    if (! f.existsAsFile()) return {};
    juce::var parsed;
    if (juce::JSON::parse(f.loadFileAsString(), parsed).failed()) return {};
    if (! parsed.isObject()) return {};
    return fromJson(parsed);
}

juce::Result UserConfig::save() const {
    auto f = configFile();
    f.getParentDirectory().createDirectory();
    auto tmp = f.getSiblingFile(f.getFileName() + ".tmp");
    if (! tmp.replaceWithText(juce::JSON::toString(toVar(), false)))
        return juce::Result::fail("Could not write " + tmp.getFullPathName());
    if (! tmp.moveFileTo(f))
        return juce::Result::fail("Could not rename tmp to " + f.getFullPathName());
    return juce::Result::ok();
}

UserConfig UserConfig::fromJson(const juce::var& v) {
    UserConfig c;
    if (v["schema_version"].isInt())
        c.schema_version = static_cast<int>(v["schema_version"]);
    if (v["profile_override"].isInt())
        c.profile_override = static_cast<int>(v["profile_override"]);
    if (v["llm"].isObject()) {
        c.llm.enabled    = static_cast<bool>(v["llm"]["enabled"]);
        c.llm.provider   = v["llm"]["provider"].toString().toStdString();
        c.llm.endpoint   = v["llm"]["endpoint"].toString().toStdString();
        c.llm.api_key    = v["llm"]["api_key"].toString().toStdString();
        c.llm.timeout_ms = static_cast<int>(v["llm"]["timeout_ms"]);
    }
    return c;
}

juce::var UserConfig::toVar() const {
    auto* root = new juce::DynamicObject();
    root->setProperty("schema_version",   schema_version);
    root->setProperty("profile_override", profile_override);
    auto* llmObj = new juce::DynamicObject();
    llmObj->setProperty("enabled",    llm.enabled);
    llmObj->setProperty("provider",   juce::String(llm.provider));
    llmObj->setProperty("endpoint",   juce::String(llm.endpoint));
    llmObj->setProperty("api_key",    juce::String(llm.api_key));
    llmObj->setProperty("timeout_ms", llm.timeout_ms);
    root->setProperty("llm", juce::var(llmObj));
    return juce::var(root);
}
