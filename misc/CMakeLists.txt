cmake_minimum_required(VERSION 3.18)

set(PROJECT_NAME "C74GenPlugin")

set(EXPORT_NAME "${PROJECT_NAME}" CACHE STRING "Name of plugin or standalone")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build. [Release]/Debug" FORCE)

option(STANDALONE_EXPORT "If ON, will export iOS app. Otherwise will export plugin(s)" OFF)
if (STANDALONE_EXPORT)
    set(EXPORT_FORMAT "Standalone")
else()
    if (APPLE)
        set(EXPORT_FORMAT VST3 AU)
    else()
        set(EXPORT_FORMAT VST3)
    endif()
endif()


project(${PROJECT_NAME} VERSION 0.0.1)

# By default we don't want Xcode schemes to be made for modules, etc
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# Support both Apple archs
set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)

add_subdirectory(JUCE)

juce_add_plugin("${PROJECT_NAME}"
    PLUGIN_NAME ${EXPORT_NAME}
    COMPANY_NAME cycling74
    BUNDLE_ID com.cycling74
    COPY_PLUGIN_AFTER_BUILD TRUE                # On MacOS, plugin will be copied to /Users/you/Library/Audio/Plug-Ins/
    PLUGIN_MANUFACTURER_CODE C74
    PLUGIN_CODE P001                            # A unique four-character plugin id with at least one upper-case character
    FORMATS ${EXPORT_FORMAT}
    PRODUCT_NAME ${EXPORT_NAME}
    IS_SYNTH FALSE                              # Currently we only create audio effects
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/exported-code/"
    "${CMAKE_CURRENT_SOURCE_DIR}/exported-code/gen_dsp"
)

set(SOURCE_FILES
    exported-code/C74_GENPLUGIN.cpp
    exported-code/C74_GENPLUGIN.h
    exported-code/gen_dsp/genlib_common_win.h
    exported-code/gen_dsp/genlib_common.h
    exported-code/gen_dsp/genlib_exportfunctions.h
    exported-code/gen_dsp/genlib_ops.h
    exported-code/gen_dsp/genlib_platform.h
    exported-code/gen_dsp/genlib.cpp
    exported-code/gen_dsp/genlib.h
    exported-code/gen_dsp/json_builder.c
    exported-code/gen_dsp/json_builder.h
    exported-code/gen_dsp/json.c
    exported-code/gen_dsp/json.h
)
if (STANDALONE_EXPORT)
    list(APPEND
        SOURCE_FILES
        Source-App/Main.cpp
        Source-App/MainComponent.cpp
        Source-App/UIComponent.cpp
        Source-App/UIComponent.h
    )
else()
    list(APPEND
        SOURCE_FILES
        Source-Plugin/PluginEditor.cpp
        Source-Plugin/PluginEditor.h
        Source-Plugin/PluginProcessor.cpp
        Source-Plugin/PluginProcessor.h
    )
endif()

juce_generate_juce_header("${PROJECT_NAME}")

target_sources("${PROJECT_NAME}" PRIVATE ${SOURCE_FILES})

# Setup our binary data as a target
# juce_add_binary_data(Assets
#     SOURCES
# )

# Set this up if you need binary assets
# set_target_properties(Assets PROPERTIES FOLDER "Targets")

target_compile_definitions("${PROJECT_NAME}"
    PUBLIC
    # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you probably don't need them.
    JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries("${PROJECT_NAME}"
    PRIVATE
    # Assets
    juce::juce_audio_utils)
