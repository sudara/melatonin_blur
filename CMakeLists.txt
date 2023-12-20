cmake_minimum_required(VERSION 3.20)

project(MelatoninBlur VERSION 1.0.0 LANGUAGES CXX
    DESCRIPTION "Fast Blurs for JUCE"
    HOMEPAGE_URL "https://github.com/sudara/melatonin_blur")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(FetchContent)
if (MelatoninBlur_IS_TOP_LEVEL)
    message(STATUS "Cloning JUCE...")

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.8
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(JUCE)

    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG v3.4.0)
    FetchContent_MakeAvailable(Catch2) # find_package equivalent

    enable_testing()
    add_executable(Tests)
    add_executable(Benchmarks)
    target_compile_features(Tests PUBLIC cxx_std_17)
    target_compile_features(Benchmarks PUBLIC cxx_std_17)

    file(GLOB_RECURSE BlurTests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    target_sources(Tests PRIVATE ${BlurTests})
    target_sources(Benchmarks PRIVATE "benchmarks/benchmarks.cpp")

    # Our executables need to know about our plugin code...
    target_include_directories(Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    target_include_directories(Benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

    juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}")

    target_link_libraries(Tests PRIVATE
        melatonin_blur
        Catch2::Catch2WithMain
        juce::juce_graphics # Image, etc
        juce::juce_gui_basics # Colour, etc
        juce::juce_audio_basics # FloatVectorOperations
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

    target_link_libraries(Benchmarks PRIVATE
        melatonin_blur
        Catch2::Catch2WithMain
        juce::juce_graphics # Image, etc
        juce::juce_gui_basics # Colour, etc
        juce::juce_audio_basics # FloatVectorOperations
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

    # Enable this once tests are happy fundamentally in CI
    # set_target_properties("${TARGET_NAME}" PROPERTIES COMPILE_WARNING_AS_ERROR ON)

    target_compile_definitions(Tests PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(Tests)
    catch_discover_tests(Benchmarks)

    if (MSVC)
        # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
        target_compile_options(Benchmarks PUBLIC $<$<CONFIG:RELEASE>:/fp:fast>)
    else ()
        # See the implications here:
        # https://stackoverflow.com/q/45685487
        target_compile_options(Benchmarks PUBLIC $<$<CONFIG:RELEASE>:-Ofast>)
        target_compile_options(Benchmarks PUBLIC $<$<CONFIG:RelWithDebInfo>:-Ofast>)
    endif ()
else ()
    if (NOT COMMAND juce_add_module)
        message(FATAL_ERROR "JUCE must be added to your project before melatonin_blur!")
    endif ()

    juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
