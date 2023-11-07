#pragma once


#if JUCE_MAC || JUCE_IOS
    #include <Accelerate/Accelerate.h>
#elif defined(PAMPLEJUCE_IPP) || defined(JUCE_IPP_AVAILABLE)
    #include "implementations/ipp_vector.h"
    #include "implementations/gin.h" // still needed for rgba windows
    #define MELATONIN_BLUR_IPP
#else
    #include "implementations/float_vector_stack_blur.h"
    #include "implementations/gin.h" // still needed for rgba windows
#endif


namespace melatonin::blur
{
    static inline std::vector<float> createFloatKernel (size_t radius)
    {
        // The kernel size is always odd
        size_t kernelSize = radius * 2 + 1;

        // This is the divisor for the kernel
        // If you are familiar with stack blur, it's the size of the stack
        auto divisor = float (radius + 1) * (float) (radius + 1);

        std::vector<float> kernel (kernelSize);

        // Manufacture the stack blur-esque kernel
        // For example, for radius of 2:
        // 1/9 2/9 3/9 2/9 1/9
        for (size_t i = 0; i < kernelSize; ++i)
        {
            auto distance = (size_t) std::abs ((int) i - (int) radius);
            kernel[i] = (float) (radius + 1 - distance) / divisor;
        }

        return kernel;
    }

    static void singleChannel (juce::Image& img, size_t radius)
    {
#if JUCE_MAC || JUCE_IOS
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto kernel = createFloatKernel (radius);

        // vdsp convolution isn't happy operating in-place, unfortunately
        auto copy = img.createCopy();
        juce::Image::BitmapData copyData (copy, juce::Image::BitmapData::readOnly);
        vImage_Buffer src = { copyData.getLinePointer (0), h, w, (size_t) data.lineStride };

        vImage_Buffer dst = { data.getLinePointer (0), h, w, (size_t) data.lineStride };
        vImageSepConvolve_Planar8 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_16U(), kvImageEdgeExtend);
#elif defined(MELATONIN_BLUR_IPP)
        ippVectorSingleChannel (img, radius);
#else
        melatonin::stackBlur::floatVectorSingleChannel (img, radius);
#endif
    }

    static void argb (juce::Image& img, size_t radius)
    {
        jassert (img.getFormat() == juce::Image::PixelFormat::ARGB);
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto kernel = createFloatKernel (radius);

#if JUCE_MAC || JUCE_IOS
        // vImageSepConvolve isn't happy operating in-place, unfortunately
        auto copy = img.createCopy();
        juce::Image::BitmapData copyData (copy, juce::Image::BitmapData::readOnly);
        vImage_Buffer src = { copyData.getLinePointer (0), h, w, (size_t) data.lineStride };

        vImage_Buffer dst = { data.getLinePointer (0), h, w, (size_t) data.lineStride };
        vImageSepConvolve_ARGB8888 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_8888 { 0, 0, 0, 0 }, kvImageEdgeExtend);
#else
        stackBlur::ginRGBA (img, radius);
#endif
    }

}
