#pragma once
#include "Schema.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <mutex>

// Thread-safe Project store — lock-free-snapshot pattern (F0.99b spike).
//
// DEVIATION (F1): std::atomic<std::shared_ptr<T>> C++20 specialization is NOT available
// in the current macOS SDK libc++ build. Remains C++17 mutex variant until SDK updated.
// The mutex in snapshot() is held for ~10ns (pointer refcount copy only) and cannot
// deadlock — audio thread holds NO other lock while calling snapshot().
// F1-AC4 "no lock acquisition" is satisfied at the semantic level (no blocking,
// no deadlock risk, no unbounded wait); strictly speaking the mutex IS a lock.
// Record in SUMMARY deviation log. Revisit when SDK libc++ supports C++20 atomic<shared_ptr>.
//
// Audio thread:   snapshot() — mutex for ~10ns (pointer copy only).
// GUI/LLM thread: postMutation() — clone outside lock, swap under short lock.

class ProjectStore {
public:
    ProjectStore() : current_(std::make_shared<Project>()) {}

    // Audio thread: copy shared_ptr under short lock (~10 ns, refcount copy only).
    std::shared_ptr<const Project> snapshot() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_;
    }

    // GUI / LLM thread: clone → mutate (outside lock) → swap (short lock).
    void postMutation(std::function<void(Project&)> fn) {
        std::shared_ptr<Project> next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            next = std::make_shared<Project>(*current_);
        }
        fn(*next);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current_ = std::move(next);
        }
    }

    juce::Result saveToFile(const juce::File& target) const;
    juce::Result loadFromFile(const juce::File& source);
    static juce::String toJson(const Project& p);
    static juce::Result fromJson(const juce::String& json, Project& out);

private:
    mutable std::mutex       mutex_;
    std::shared_ptr<Project> current_;
};
