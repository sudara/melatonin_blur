// These defines are required to load juce_core in
// assuming we later need native functionality like direct2d, etc
// juce_core must be loaded *first* so we can detect platform/version
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS && JUCE_MAJOR_VERSION >= 8
    #include "melatonin/implementations/direct2d.cpp"
#endif

#include "melatonin/cached_blur.cpp"
#include "melatonin/internal/cached_shadows.cpp"
#include "melatonin/internal/rendered_single_channel_shadow.cpp"

#if RUN_MELATONIN_BENCHMARKS
    #include "benchmarks/benchmarks.cpp"
#endif

#if RUN_MELATONIN_TESTS
    #include "tests/blur_implementations.cpp"
    #include "tests/drop_shadow.cpp"
    #include "tests/inner_shadow.cpp"
    #include "tests/shadow_scaling.cpp"
    #include "tests/path_with_shadows.cpp"
    #include "tests/text_shadow.cpp"
#endif
