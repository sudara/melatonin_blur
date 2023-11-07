#pragma once
#include "juce_graphics/juce_graphics.h"

#if JUCE_MAC
    #include <Accelerate/Accelerate.h>
#endif
namespace melatonin::stackBlur
{
#if JUCE_MAC
    static void tentBlurSingleChannel (juce::Image& img, unsigned int radius)
    {
        const unsigned int w = (unsigned int) img.getWidth();
        const unsigned int h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        vImage_Buffer src = { data.getLinePointer (0), h, w, (size_t) data.lineStride };
        vImage_Buffer dst = { data.getLinePointer (0), h, w, (size_t) data.lineStride };
        vImageTentConvolve_Planar8 (
            &src,
            &dst,
            nullptr,
            0,
            0,
            radius * 2 + 1,
            radius * 2 + 1,
            0,
            kvImageEdgeExtend);
    }
#endif
}
