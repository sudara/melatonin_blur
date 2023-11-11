#pragma once

#if JUCE_MAC || JUCE_IOS
    #include <Accelerate/Accelerate.h>
#elif defined(PAMPLEJUCE_IPP) || defined(JUCE_IPP_AVAILABLE)
    #include "implementations/ipp_vector.h"
    #define MELATONIN_BLUR_IPP
#else
    #include "implementations/float_vector_stack_blur.h"
#endif

#include "implementations/gin.h" // still needed for rgba

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
        melatonin::blur::juceFloatVectorSingleChannel (img, radius);
#endif
    }

    static void argb (juce::Image& img, size_t radius)
    {
        jassert (img.getFormat() == juce::Image::PixelFormat::ARGB);

#if JUCE_MAC || JUCE_IOS
        auto kernel = createFloatKernel (radius);

        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        // vImageSepConvolve isn't happy operating in-place
        auto copy = img.createCopy();
        juce::Image::BitmapData copyData (copy, juce::Image::BitmapData::readWrite);

        vImage_Buffer src = { copyData.getLinePointer (0), h, w, (size_t) copyData.lineStride };
        vImage_Buffer dst = { data.getLinePointer (0), h, w, (size_t) data.lineStride };
        vImageSepConvolve_ARGB8888 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_8888 { 0, 0, 0, 0 }, kvImageEdgeExtend);
#else
        stackBlur::ginRGBA (img, radius);
#endif
    }

    static void argb (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        jassert (srcImage.getFormat() == juce::Image::PixelFormat::ARGB);
#if JUCE_MAC || JUCE_IOS
        auto kernel = createFloatKernel (radius);

        const auto w = (unsigned int) srcImage.getWidth();
        const auto h = (unsigned int) srcImage.getHeight();
        juce::Image::BitmapData srcData (srcImage, juce::Image::BitmapData::readWrite);
        juce::Image::BitmapData dstData (dstImage, juce::Image::BitmapData::readWrite);

        // vImageSepConvolve isn't happy operating in-place
        vImage_Buffer src = { srcData.getLinePointer (0), h, w, (size_t) srcData.lineStride };
        vImage_Buffer dst = { dstData.getLinePointer (0), h, w, (size_t) dstData.lineStride };
        vImageSepConvolve_ARGB8888 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_8888 { 0, 0, 0, 0 }, kvImageEdgeExtend);
#else
        stackBlur::ginRGBA (srcImage, radius);
#endif
    }
}

namespace melatonin
{
    class CachedBlur
    {
    public:
        explicit CachedBlur (size_t r) : radius (r)
        {
            jassert (radius > 0);
        }

        // we are passing the source by value here
        // (but it's a value object of sorts since its reference counted)
        void update (juce::Image newSource)
        {
            if (newSource != src)
            {
                jassert (newSource.isValid());
                src = newSource;

                // the first time the blur is created, a copy is needed
                // so we are passing correct dimensions, etc to the blur algo
                dst = src.createCopy();
                melatonin::blur::argb (src, dst, radius);
            }
        }

        juce::Image& render (juce::Image& newSource)
        {
            update (newSource);
            return dst;
        }

        juce::Image& render()
        {
            // You either need to have called update or rendered with a src!
            jassert (dst.isValid());
            return dst;
        }

    private:
        // juce::Images are value objects, reference counted behind the scenes
        // We want to store a reference to the src so we can compare on render
        // And we actually are the owner of the dst
        juce::Image src = juce::Image();
        juce::Image dst = juce::Image();
        size_t radius;
    };
}
