#if RUN_MELATONIN_BENCHMARKS
    #include "benchmarks/benchmarks.cpp"
#endif

#if RUN_MELATONIN_TESTS
    #include "tests/blur_implementations.cpp"
    #include "tests/drop_shadow.cpp"
    #include "tests/inner_shadow.cpp"
    #include "tests/shadow_scaling.cpp"
    #include "tests/path_with_shadows.cpp"
#endif
