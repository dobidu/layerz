cmake_minimum_required(VERSION 3.22)
project(LAYERZ VERSION 0.0.1 LANGUAGES CXX)

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
)

target_sources(LAYERZ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(LAYERZ PUBLIC cxx_std_17)

target_compile_definitions(LAYERZ PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=1    # Required: JUCE GPLv3 free tier license obligation
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(LAYERZ PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_basics
)

# Mark JUCE headers as SYSTEM so -Werror does not apply to JUCE's own warnings.
# JUCE generates warnings on newer GCC/Clang/MSVC that are outside project control.
target_include_directories(LAYERZ SYSTEM PRIVATE
    ${juce_SOURCE_DIR}
    ${juce_SOURCE_DIR}/modules
)

# -Werror scoped to project sources only (JUCE excluded via SYSTEM includes above)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(LAYERZ PRIVATE -Wall -Wextra -Werror)
elseif(MSVC)
    target_compile_options(LAYERZ PRIVATE /W4 /WX)
endif()
