#pragma once

#include "../blur_options.h"
#include "juce_graphics/juce_graphics.h"

namespace melatonin::blur
{
    constexpr float direct2DRadiusToStdDev = 1.0f / 1.25f;
    // Below this, D2D setup cost beats the GPU work for cached shadows.
    constexpr int direct2DMinimumSingleChannelPixels = 16 * 16;

    class Direct2DSingleChannelBlur
    {
    public:
        Direct2DSingleChannelBlur();
        ~Direct2DSingleChannelBlur();

        Direct2DSingleChannelBlur (const Direct2DSingleChannelBlur&) = delete;
        Direct2DSingleChannelBlur& operator= (const Direct2DSingleChannelBlur&) = delete;

        bool render (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
        void reset();

    private:
        struct Pimpl;
        std::unique_ptr<Pimpl> pimpl;
    };

    bool direct2DSingleChannel (juce::Image& img, size_t radius);
    bool direct2DSingleChannel (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
    bool direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius);
}
