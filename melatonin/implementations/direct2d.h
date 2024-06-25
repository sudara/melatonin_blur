#pragma once
#include "juce_graphics/juce_graphics.h"

namespace melatonin::blur
{
    // stdDev = radius / sqrt(2)
    constexpr float radiusToStdDev = 1.0f / 1.4142135623730951f;
    static inline void direct2DSingleChannel (juce::Image& img, size_t radius);
    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
}
