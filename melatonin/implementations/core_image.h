#pragma once
#include <juce_graphics/juce_graphics.h>

namespace melatonin::blur
{
    void coreImageSingleChannel (juce::Image& img, size_t radius);
    void coreImageARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
}
