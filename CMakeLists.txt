cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PiMax VERSION 1.1.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE INTERNAL "")
if (PRODUCTION_BUILD)
    if (WIN32)
        set(CMAKE_SYSTEM_VERSION 7 CACHE INTERNAL "")
    elseif(APPLE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
    endif()
    set(CMAKE_BUILD_TYPE Release CACHE INTERNAL "")
else()
    set(PRODUCTION_BUILD 0)
endif()
message("PRODUCTION_BUILD: ${PRODUCTION_BUILD}")

if (NOT INSTALL)
    set(INSTALL OFF)
endif()

include(Dependencies.cmake)
fetch_dependencies()

if(NOT APPLE)
    set(USE_PFFFT 1)
    # add_subdirectory(modules/pffft)
else()
    set(USE_PFFFT 0)
endif()

set(JUCE_FORMATS VST3 AU)

if(BUILD_VST2)
    juce_set_vst2_sdk_path("./vst2_sdk")
    list(APPEND JUCE_FORMATS VST)
endif()

juce_add_plugin(PiMax
    COMPANY_NAME "Arboreal Audio"
    COMPANY_COPYRIGHT "(c) 2024 Arboreal Audio, LLC"
    COPY_PLUGIN_AFTER_BUILD ${INSTALL}
    PLUGIN_MANUFACTURER_CODE Arau
    PLUGIN_CODE Hmsj
    FORMATS ${JUCE_FORMATS}
    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion Mastering
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "PiMax"
    NEEDS_CURL TRUE
    HARDENED_RUNTIME_ENABLED TRUE)

clap_juce_extensions_plugin(TARGET PiMax
    CLAP_ID "com.ArborealAudio.PiMax"
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
    CLAP_FEATURES audio-effect distortion mastering)

juce_generate_juce_header(PiMax)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)
target_sources(PiMax PRIVATE ${SourceFiles})

target_include_directories(PiMax PRIVATE
    "modules/farbot/include"
)

juce_add_modules(
    "modules/Gin/modules/gin"
    "modules/Gin/modules/gin_gui"
    "modules/Arbor_modules")

target_compile_definitions(PiMax
PUBLIC
    PRODUCTION_BUILD=${PRODUCTION_BUILD}
    JUCE_USE_PFFFT=${USE_PFFFT}
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=0
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
    AWS_API_KEY="$ENV{AWS_API_KEY}"
)

juce_add_binary_data(BinaryData SOURCES
    "src/Resources/arboreal tree.svg"
    "src/Resources/Hamburger_icon.svg"
    "src/Resources/fonts/Sora-Regular.ttf"
    "src/Resources/fonts/Sora-Thin.ttf"
    "src/Resources/fonts/Sora-SemiBold.ttf"
    "src/Presets/Factory Presets/Bass Fattener.aap"
    "src/Presets/Factory Presets/Bass Quack.aap"
    "src/Presets/Factory Presets/Broken Gear.aap"
    "src/Presets/Factory Presets/Default.aap"
    "src/Presets/Factory Presets/Drum Crusher.aap"
    "src/Presets/Factory Presets/Drum Expander.aap"
    "src/Presets/Factory Presets/Guitar Stompbox.aap"
    "src/Presets/Factory Presets/Kick Enhancer.aap"
    "src/Presets/Factory Presets/Loudness Curve.aap"
    "src/Presets/Factory Presets/Snare Enhancer.aap"
    "src/Presets/Factory Presets/Subtle Warmth.aap"
    "src/Presets/Factory Presets/Vocal Polisher.aap"
)

#-fPIC and static linker flags, & PFFFT for Linux
if (UNIX AND NOT APPLE)
    set_target_properties(BinaryData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    target_link_libraries(PiMax PRIVATE "-static-libgcc" "-static-libstdc++")
    set_target_properties(PFFFT PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()

target_link_libraries(PiMax
PRIVATE
    BinaryData
    juce::juce_audio_utils
    juce::juce_dsp
    gin
    gin_gui
    Arbor_modules
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)

if (NOT APPLE)
    target_link_libraries(PiMax PRIVATE
    PFFFT
    juce::juce_opengl)
endif()

