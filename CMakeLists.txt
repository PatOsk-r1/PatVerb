cmake_minimum_required(VERSION 3.25)
project(PatVerb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PATVERB_BUILD_PLUGIN "Build the plugin (VST3 + Standalone)" ON)
option(PATVERB_BUILD_TESTS "Build the unit tests" ON)
option(PATVERB_COPY_VST3 "Copy the VST3 to PATVERB_VST3_DIR after building (dist preset)" OFF)
set(PATVERB_VST3_DIR "C:/Program Files/Common Files/VST3" CACHE PATH "VST3 folder FL Studio scans")

set(PV_THIRD_PARTY ${CMAKE_SOURCE_DIR}/third_party)
if(NOT EXISTS ${PV_THIRD_PARTY}/JUCE/CMakeLists.txt)
    message(FATAL_ERROR "Dependencies missing. Run: powershell -ExecutionPolicy Bypass -File tools/fetch-deps.ps1")
endif()

add_library(patverb_warnings INTERFACE)
if(MSVC)
    target_compile_options(patverb_warnings INTERFACE /W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
    target_compile_options(patverb_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# engine: pure C++ (no JUCE), fully testable
add_library(patverb_engine STATIC
    src/engine/Params.cpp
    src/engine/Reverb.cpp
)
target_include_directories(patverb_engine PUBLIC src)
target_link_libraries(patverb_engine PRIVATE patverb_warnings)
if(MSVC)
    target_compile_options(patverb_engine PRIVATE $<$<NOT:$<CONFIG:Debug>>:/O2 /fp:fast>)
endif()

if(PATVERB_BUILD_PLUGIN)
    add_subdirectory(${PV_THIRD_PARTY}/JUCE ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)

    juce_add_plugin(PatVerb
        PRODUCT_NAME "PatVerb"
        COMPANY_NAME "PatWave"
        BUNDLE_ID "com.patwave.patverb"
        VERSION ${PROJECT_VERSION}
        PLUGIN_MANUFACTURER_CODE Ptwv
        PLUGIN_CODE Pvb1
        FORMATS VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Fx Reverb
        COPY_PLUGIN_AFTER_BUILD ${PATVERB_COPY_VST3}
        VST3_COPY_DIR "${PATVERB_VST3_DIR}")

    target_sources(PatVerb PRIVATE
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginEditor.cpp
        src/plugin/Presets.cpp
        src/plugin/ui/Theme.cpp
        src/plugin/ui/Galaxy.cpp
        src/plugin/ui/Controls.cpp
    )
    target_compile_definitions(PatVerb PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(PatVerb
        PRIVATE patverb_engine patverb_warnings juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endif()

if(PATVERB_BUILD_TESTS)
    enable_testing()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(${PV_THIRD_PARTY}/googletest ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
    add_executable(patverb_tests tests/ReverbTests.cpp)
    target_link_libraries(patverb_tests PRIVATE patverb_engine patverb_warnings GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(patverb_tests DISCOVERY_MODE PRE_TEST)
endif()
