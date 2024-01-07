
#if JUCE_USE_SSE_INTRINSICS
    #include "prefix_sum_sse2.h"
#elif JUCE_USE_ARM_NEON
    #include "prefix_sum_neon.h"
#endif

namespace melatonin::blur
{
    static void prefixSumSingleChannelSIMD (juce::Image& img, unsigned int radius)
    {
#if JUCE_USE_SSE_INTRINSICS
        prefixSumSingleChannelSSE2 (img, radius);
#elif JUCE_USE_ARM_NEON
        prefixSumSingleChannelNeon (img, radius);
#endif
    }
}
