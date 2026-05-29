#pragma once
#include "Schema.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <mutex>

// Thread-safe Project store — lock-free-snapshot pattern (F0.99b spike).
//
// C++17 note: std::atomic<std::shared_ptr<T>> requires trivially-copyable T (C++20 only).
// F0 implementation: short mutex around shared_ptr refcount copy on audio thread.
// The mutex is held ONLY for a pointer copy (~10 ns) or a pointer swap (~10 ns).
// No user-level lock is held during mutation work (clone + fn application).
// Upgrade path: C++20 std::atomic<std::shared_ptr<Project>> removes the mutex entirely.
//
// Audio thread:   snapshot() — acquires mutex for ~10 ns (refcount copy), never blocks on I/O.
// GUI/LLM thread: postMutation() — clones outside lock, acquires mutex briefly for swap only.

class ProjectStore {
public:
    ProjectStore() : current_(std::make_shared<Project>()) {}

    // Audio thread: copy shared_ptr under short lock (~10 ns, refcount increment only).
    std::shared_ptr<const Project> snapshot() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_;
    }

    // GUI / LLM thread: clone → mutate (outside lock) → swap (short lock).
    // fn is called without holding the lock — safe for slow/complex operations.
    void postMutation(std::function<void(Project&)> fn) {
        // Clone current under lock (brief)
        std::shared_ptr<Project> next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            next = std::make_shared<Project>(*current_);
        }
        // Mutate the clone with no lock held
        fn(*next);
        // Swap in the new version under lock (brief)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = std::move(next);
        }
    }

    // GUI thread: atomic write (tmp → rename) prevents corrupt files on crash.
    juce::Result saveToFile(const juce::File& target) const;

    // GUI thread: deserialise JSON → replace current project.
    // Returns error on: malformed JSON, missing fields, unsupported schema_version.
    // Unknown extra JSON fields are silently skipped (forward compat).
    juce::Result loadFromFile(const juce::File& source);

    static juce::String toJson(const Project& p);
    static juce::Result fromJson(const juce::String& json, Project& out);

private:
    mutable std::mutex          mutex_;
    std::shared_ptr<Project>    current_;
};
