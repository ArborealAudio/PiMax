function(fetch_dependencies)
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)

  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.11
  )
  FetchContent_MakeAvailable(juce)

  FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG main
  )
  FetchContent_MakeAvailable(clap-juce-extensions)
  
endfunction()
