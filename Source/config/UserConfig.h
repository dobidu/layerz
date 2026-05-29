#pragma once
#include <juce_core/juce_core.h>
#include <string>

// Installation-level settings — NOT stored in .layerz project files.
// Persisted to platform config dir (see UserConfig::configFile()).
// LLM bridge is opt-in, disabled by default: LAYERZ is fully functional without it.
// Versioned (schema_version) for the same migration reasons as Project.

struct LLMConfig {
    bool        enabled     = false;       // opt-in; user must explicitly enable
    std::string provider    = "anthropic"; // provider-neutral; first target
    std::string endpoint;                  // empty = use provider default
    std::string api_key;                   // never logged, never written to .layerz
    int         timeout_ms  = 30000;
};

struct UserConfig {
    int       schema_version = 1;

    // Profile::AUTO = infer from host context at runtime (default)
    // Override to PLUGIN or STANDALONE to force a specific cap set
    int       profile_override = 0;  // 0=AUTO, 1=PLUGIN, 2=STANDALONE

    LLMConfig llm;

    // Utility
    static juce::File configFile();
    static UserConfig load();            // returns defaults if file missing or malformed
    juce::Result      save() const;      // atomic write (tmp → rename)

    static UserConfig fromJson(const juce::var& v);
    juce::var         toVar() const;
};
