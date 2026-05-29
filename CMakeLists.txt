cmake_minimum_required(VERSION 3.22)
project(LAYERZ VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Verify latest stable tag at https://github.com/juce-framework/JUCE/releases before building
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LAYERZ
    COMPANY_NAME                "LAYERZ"
    PLUGIN_MANUFACTURER_CODE    "Lyrz"
    PLUGIN_CODE                 "Lyrz"
    FORMATS                     VST3 AU LV2 Standalone
    PRODUCT_NAME                "LAYERZ"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    LV2URI                      "urn:layerz:plugin"
)

target_sources(LAYERZ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/model/ProjectStore.cpp
    Source/config/UserConfig.cpp
    Source/audio/Clock.cpp
)

target_compile_features(LAYERZ PUBLIC cxx_std_17)

target_compile_definitions(LAYERZ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(LAYERZ PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_basics
)

# Optional JUCE AudioPluginHost for local VST3/AU testing (not built by default)
add_subdirectory(
    ${juce_SOURCE_DIR}/extras/AudioPluginHost
    ${CMAKE_BINARY_DIR}/AudioPluginHost
    EXCLUDE_FROM_ALL
)

# Apply strict warnings only to project source files — NOT to JUCE module sources.
# target_compile_options on the LAYERZ target would hit JUCE's compiled modules too,
# which contain their own compiler-version-specific warnings we cannot fix.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/model/ProjectStore.cpp
        Source/config/UserConfig.cpp
        Source/audio/Clock.cpp
        PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror"
    )
elseif(MSVC)
    set_source_files_properties(
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/model/ProjectStore.cpp
        Source/config/UserConfig.cpp
        Source/audio/Clock.cpp
        PROPERTIES COMPILE_OPTIONS "/W4;/WX"
    )
endif()
