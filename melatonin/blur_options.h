#pragma once

#include "juce_graphics/juce_graphics.h"

// CMake (or user) opts in/out. Default on; the platform check below gates it.
#ifndef MELATONIN_BLUR_USE_DIRECT2D
 #define MELATONIN_BLUR_USE_DIRECT2D 1
#endif

// Direct2D only exists on Windows + JUCE 8, so AND the platform in here once
// so every call site can just do `#if MELATONIN_BLUR_USE_DIRECT2D`.
#if MELATONIN_BLUR_USE_DIRECT2D && JUCE_WINDOWS && JUCE_MAJOR_VERSION >= 8
 #undef MELATONIN_BLUR_USE_DIRECT2D
 #define MELATONIN_BLUR_USE_DIRECT2D 1
#else
 #undef MELATONIN_BLUR_USE_DIRECT2D
 #define MELATONIN_BLUR_USE_DIRECT2D 0
#endif

namespace melatonin::blur
{
   #if MELATONIN_BLUR_USE_DIRECT2D
    inline std::atomic<bool>& direct2DEnabledState()
    {
        static std::atomic<bool> enabled { true };
        return enabled;
    }

    inline void setDirect2DEnabled (bool enabled)
    {
        direct2DEnabledState().store (enabled);
    }

    [[nodiscard]] inline bool isDirect2DEnabled()
    {
        return direct2DEnabledState().load();
    }
   #else
    inline void setDirect2DEnabled (bool) {}

    [[nodiscard]] inline bool isDirect2DEnabled()
    {
        return false;
    }
   #endif
}
