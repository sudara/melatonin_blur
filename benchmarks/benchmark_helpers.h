#pragma once

// GitHub-hosted Windows runners have no GPU passthrough, so Direct2D falls back
// to software rendering (WARP). In that environment D2D is ~10× slower than the
// CPU path and the per-sample variance exceeds the mean — i.e. the numbers are
// noise. The CI workflow skips the Benchmarks step when MELATONIN_BLUR_USE_DIRECT2D
// is enabled for this reason; run benchmarks on real hardware to get a signal.

#if MELATONIN_BLUR_USE_DIRECT2D
namespace
{
    struct ScopedDirect2DBenchmarkSetting
    {
        explicit ScopedDirect2DBenchmarkSetting (bool enabled)
            : previous (melatonin::blur::isDirect2DEnabled())
        {
            melatonin::blur::setDirect2DEnabled (enabled);
        }

        ~ScopedDirect2DBenchmarkSetting()
        {
            melatonin::blur::setDirect2DEnabled (previous);
        }

        bool previous = false;
    };
}
#endif
