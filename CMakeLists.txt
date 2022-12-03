cmake_minimum_required(VERSION 3.15)

if (WIN32)
    set(CMAKE_SYSTEM_VERSION 7 CACHE TYPE INTERNAL FORCE)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

project(PiMax VERSION 1.1.1)

add_subdirectory(Source/modules/juce)
add_subdirectory(Source/modules/clap-juce-extensions)

juce_set_vst2_sdk_path("$ENV{HOME}/vst2_sdk")

set(JUCE_FORMATS VST3 AU)

if(UNIX AND NOT APPLE)
    list(APPEND JUCE_FORMATS LV2)
endif()

if(TARGET juce_vst2_sdk)
    list(APPEND JUCE_FORMATS VST)
endif()

juce_add_plugin(PiMax
    COMPANY_NAME "Arboreal Audio"
    COMPANY_COPYRIGHT "(c) 2022 Arboreal Audio, LLC"
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Arau
    PLUGIN_CODE Hmsj
    FORMATS ${JUCE_FORMATS}
    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion Mastering
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "PiMax"
    HARDENED_RUNTIME_ENABLED TRUE)

clap_juce_extensions_plugin(TARGET PiMax
    CLAP_ID "com.ArborealAudio.PiMax"
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
    CLAP_FEATURES audio-effect distortion mastering)

if ((UNIX AND NOT APPLE) OR WIN32)
    if (UNIX AND NOT APPLE)
        list(APPEND CMAKE_MODULE_PATH "/opt/intel/oneapi/ipp/latest/lib/cmake/ipp")
    endif()
    find_package(IPP REQUIRED)
    if(${IPP_FOUND})
        message("FOUND: IPP Libs")
        add_compile_definitions(_IPP_SEQUENTIAL_STATIC=1)
        target_include_directories(PiMax PUBLIC
        "${ipp_cmake_package_dir}/${IPP_INC_REL_PATH}")
        target_link_libraries(PiMax ${IPP_LIBRARIES})
    else()
        message("NOT FOUND: IPP Libs")
    endif()
endif()

juce_generate_juce_header(PiMax)

target_sources(PiMax
PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DownloadManager.h
    Source/OnlineActivation.h)
    
add_subdirectory(Source/UI)
add_subdirectory(Source/Processors)
add_subdirectory(Source/Presets)
    
target_include_directories(PiMax PRIVATE "Source/modules/farbot/include")
juce_add_modules(
    "$ENV{HOME}/modules/Gin/modules/gin"
    "$ENV{HOME}/modules/Gin/modules/gin_gui"
    "$ENV{HOME}/modules/Arbor_modules")

target_compile_definitions(PiMax
PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=0)

juce_add_binary_data(BinaryData SOURCES
    "Source/Resources/arboreal tree.svg"
    "Source/Resources/Hamburger_icon.svg"
    "Source/Resources/fonts/Sora-Regular.ttf"
    "Source/Resources/fonts/Sora-Thin.ttf"
    "Source/Resources/fonts/Sora-SemiBold.ttf"
    "Source/Presets/Factory Presets/Bass Fattener.aap"
    "Source/Presets/Factory Presets/Bass Quack.aap"
    "Source/Presets/Factory Presets/Broken Gear.aap"
    "Source/Presets/Factory Presets/Default.aap"
    "Source/Presets/Factory Presets/Drum Crusher.aap"
    "Source/Presets/Factory Presets/Drum Expander.aap"
    "Source/Presets/Factory Presets/Guitar Stompbox.aap"
    "Source/Presets/Factory Presets/Kick Enhancer.aap"
    "Source/Presets/Factory Presets/Loudness Curve.aap"
    "Source/Presets/Factory Presets/Snare Enhancer.aap"
    "Source/Presets/Factory Presets/Subtle Warmth.aap"
    "Source/Presets/Factory Presets/Vocal Polisher.aap")

#-fPIC flag for proper Linux compile
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(PiMax
PRIVATE
    BinaryData
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_product_unlocking
    juce::juce_cryptography
    gin
    gin_gui
    Arbor_modules
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if (NOT APPLE)
    target_link_libraries(PiMax PRIVATE
    juce::juce_opengl)
endif()