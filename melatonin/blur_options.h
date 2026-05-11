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

    // Monotonic counter bumped whenever the D2D enabled state actually changes.
    // CachedShadows uses this to self-invalidate so the next render() picks up
    // the new backend without callers needing to clear caches themselves.
    inline std::atomic<std::uint64_t>& direct2DGenerationCounter()
    {
        static std::atomic<std::uint64_t> gen { 0 };
        return gen;
    }

    inline void setDirect2DEnabled (bool enabled)
    {
        if (direct2DEnabledState().exchange (enabled) != enabled)
            direct2DGenerationCounter().fetch_add (1, std::memory_order_relaxed);
    }

    [[nodiscard]] inline bool isDirect2DEnabled()
    {
        return direct2DEnabledState().load();
    }

    [[nodiscard]] inline std::uint64_t direct2DConfigGeneration()
    {
        return direct2DGenerationCounter().load (std::memory_order_relaxed);
    }
   #else
    inline void setDirect2DEnabled (bool) {}

    [[nodiscard]] inline bool isDirect2DEnabled()
    {
        return false;
    }

    [[nodiscard]] inline std::uint64_t direct2DConfigGeneration()
    {
        return 0;
    }
   #endif
}
