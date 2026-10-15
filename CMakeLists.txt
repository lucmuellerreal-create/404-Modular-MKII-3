cmake_minimum_required(VERSION 3.22)
project(Modular404 VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(Modular404
    COMPANY_NAME "Independent Audio"
    BUNDLE_ID "audio.independent.modular404"
    PLUGIN_MANUFACTURER_CODE IndA
    PLUGIN_CODE M404
    FORMATS VST3
    PRODUCT_NAME "404 Modular MKII"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Modular404 PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_definitions(Modular404 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Modular404 PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
