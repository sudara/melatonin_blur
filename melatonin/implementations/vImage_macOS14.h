#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "vImage.h"

namespace melatonin::blur
{
    void vImageARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
}
