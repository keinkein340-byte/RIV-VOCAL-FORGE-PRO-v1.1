cmake_minimum_required(VERSION 3.22)
project(RIV_VOCAL_FORGE VERSION 1.1.0)

option(RIV_FETCH_JUCE "Download JUCE during configuration" ON)
if(RIV_FETCH_JUCE)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
else()
  message(FATAL_ERROR "Set RIV_FETCH_JUCE=ON or provide JUCE through your own CMake integration.")
endif()

juce_add_plugin(RIV_VOCAL_FORGE
  COMPANY_NAME "RIV Audio"
  BUNDLE_ID "com.rivaudio.vocalforge"
  PLUGIN_MANUFACTURER_CODE Riva
  PLUGIN_CODE RvF1
  PRODUCT_NAME "RIV VOCAL FORGE"
  DESCRIPTION "RIV VOCAL FORGE PRO adaptive vocal processor"
  FORMATS VST3 Standalone
  VST3_CATEGORIES Fx
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(RIV_VOCAL_FORGE)
target_sources(RIV_VOCAL_FORGE PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)
target_compile_features(RIV_VOCAL_FORGE PRIVATE cxx_std_17)
target_compile_definitions(RIV_VOCAL_FORGE PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(RIV_VOCAL_FORGE PRIVATE
  juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
