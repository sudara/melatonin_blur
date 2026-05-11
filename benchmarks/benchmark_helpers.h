#pragma once

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
