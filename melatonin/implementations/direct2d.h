#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#include <juce_graphics/native/juce_DirectX_windows.h>
#include <juce_graphics/native/juce_Direct2DImage_windows.h>

namespace melatonin::blur
{


    static inline void direct2DSingleChannel (juce::Image& img, size_t radius)
    {
    }

    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {

    }
}
