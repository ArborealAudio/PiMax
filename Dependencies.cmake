function(fetch_dependencies)
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)

  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.11
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(juce)

  FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 8d0754f5d6ca1e95bc207b7743c04ebd7dc17e88
  )
  FetchContent_MakeAvailable(clap-juce-extensions)

  FetchContent_Declare(pffft
    GIT_REPOSITORY https://github.com/marton78/pffft.git
    GIT_TAG e0bf595c98ded55cc457a371c1b29c8cab552628
    GIT_SUBMODULES ""
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(pffft)
  
endfunction()
