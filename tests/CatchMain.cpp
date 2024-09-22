// All test files are included in the executable via the Glob in CMakeLists.txt

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_gui_basics/juce_gui_basics.h"

//static std::ostream& operator<< (std::ostream& os, juce::Colour const& c)
//{
//    os << c.toString().toStdString();
//    return os;
//}

#define CATCH_CONFIG_EXTERNAL_INTERFACES
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
