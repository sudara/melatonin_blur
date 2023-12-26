#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::blur
{
    std::vector<float> createFloatKernel (size_t radius);

    void vImageSingleChannel (juce::Image& img, size_t radius);

    // currently unused, may be benchmarked vs. drawImageAt
    juce::Image convertToARGB (juce::Image& src, juce::Colour color);

    void tentBlurSingleChannel (juce::Image& img, unsigned int radius);
}
