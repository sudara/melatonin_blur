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
    add_executable(Tests ${TestFiles})
    target_compile_features(Tests PUBLIC cxx_std_17)

    target_sources(Tests PRIVATE "tests/blur_implementations.cpp" "tests/drop_shadow.cpp" "tests/inner_shadow.cpp")

    # Our test executable also wants to know about our plugin code...
    target_include_directories(Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

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

    # Enable this once tests are happy fundamentally in CI
    # set_target_properties("${TARGET_NAME}" PROPERTIES COMPILE_WARNING_AS_ERROR ON)

    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(Tests)
else ()
    message(WARNING "This CMake config is just for CI tests.\nSubmit an Issue / PR if you want more CMake support: https://github.com/sudara/melatonin_blur ")
endif ()
