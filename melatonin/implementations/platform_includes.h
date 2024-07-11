#pragma once

// These defines are required to load juce_core in
// assuming we later need native functionality like direct2d, etc
// juce_core must be loaded *first* so we can detect platform/version
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS && JUCE_MAJOR_VERSION >= 8
    #include "direct2d.cpp"
#endif
