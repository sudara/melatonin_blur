#if JUCE_WINDOWS
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
