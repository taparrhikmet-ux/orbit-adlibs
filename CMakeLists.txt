cmake_minimum_required(VERSION 3.22)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
project(OrbitAdlibs VERSION 0.1.0 LANGUAGES C CXX)
if(NOT APPLE)
    message(FATAL_ERROR "This project is configured for macOS. Build it on a Mac.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(OrbitAdlibs
    COMPANY_NAME "Orbit Audio"
    BUNDLE_ID "audio.orbit.adlibs"
    PLUGIN_MANUFACTURER_CODE Orbt
    PLUGIN_CODE Oadl
    FORMATS VST3 AU
    PRODUCT_NAME "Orbit Adlibs"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Modulation Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect)
juce_generate_juce_header(OrbitAdlibs)
target_sources(OrbitAdlibs PRIVATE
    Source/PluginProcessor.h Source/PluginProcessor.cpp
    Source/PluginEditor.h Source/PluginEditor.cpp)
target_compile_definitions(OrbitAdlibs PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(OrbitAdlibs PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
