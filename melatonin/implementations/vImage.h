#pragma once
#include "Accelerate/Accelerate.h"
#include "juce_gui_basics/juce_gui_basics.h"

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

    static inline void vImageSingleChannel (juce::Image& img, size_t radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();
        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto kernel = createFloatKernel (radius);

        // vdsp convolution isn't happy operating in-place, unfortunately
        auto copy = img.createCopy();
        juce::Image::BitmapData copyData (copy, juce::Image::BitmapData::readOnly);
        vImage_Buffer src = { copyData.getLinePointer (0), h, w, (size_t) data.lineStride };

        vImage_Buffer dst = { data.getLinePointer (0), h, w, (size_t) data.lineStride };

        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunguarded-availability-new")
        vImageSepConvolve_Planar8 (&src, &dst, nullptr, 0, 0, kernel.data(), (unsigned int) kernel.size(), kernel.data(), (unsigned int) kernel.size(), 0, Pixel_16U(), kvImageEdgeExtend);
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
    }

    static inline std::optional<uint8_t> uniformBorderValue (const juce::Image::BitmapData& data, int band)
    {
        band = juce::jmin (band, data.width);
        const auto value = *data.getLinePointer (0);
        const auto matches = [value] (const uint8_t* first, int count) {
            return std::all_of (first, first + count, [value] (uint8_t pixel) { return pixel == value; });
        };

        for (int y = 0; y < data.height; ++y)
        {
            const auto* row = data.getLinePointer (y);
            const bool wholeRow = y < band || y >= data.height - band;

            if (wholeRow ? ! matches (row, data.width) : ! (matches (row, band) && matches (row + data.width - band, band)))
                return {};
        }

        return value;
    }

    static inline bool vImageBoxSingleChannel (juce::Image& img, size_t radius)
    {
        // Two box filters of width radius + 1 produce the triangular kernel. vImage requires an odd width.
        if (radius % 2 != 0)
            return false;

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);
        // A uniform border lets both passes use the edge colour for samples outside the image.
        const auto border = uniformBorderValue (data, (int) radius / 2);

        if (! border)
            return false;

        const auto w = (vImagePixelCount) data.width;
        const auto h = (vImagePixelCount) data.height;
        const auto stride = (size_t) data.lineStride;

        juce::HeapBlock<uint8_t> scratch (stride * h);
        vImage_Buffer image = { data.getLinePointer (0), h, w, stride };
        vImage_Buffer temp = { scratch.get(), h, w, stride };

        const auto boxSize = (uint32_t) radius + 1;
        return vImageBoxConvolve_Planar8 (&image, &temp, nullptr, 0, 0, boxSize, boxSize, *border, kvImageBackgroundColorFill) == kvImageNoError
               && vImageBoxConvolve_Planar8 (&temp, &image, nullptr, 0, 0, boxSize, boxSize, *border, kvImageBackgroundColorFill) == kvImageNoError;
    }

    // currently unused, may be benchmarked vs. drawImageAt
    [[maybe_unused]] static juce::Image convertToARGB (juce::Image& src, juce::Colour color)
    {
        jassert (src.getFormat() == juce::Image::SingleChannel);
        juce::Image dst (juce::Image::ARGB, src.getWidth(), src.getHeight(), true);
        juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);
        vImage_Buffer alphaBuffer = { srcData.getLinePointer (0), static_cast<vImagePixelCount> (src.getHeight()), static_cast<vImagePixelCount> (src.getWidth()), static_cast<size_t> (srcData.lineStride) };
        vImage_Buffer dstBuffer = { dstData.getLinePointer (0), static_cast<vImagePixelCount> (dst.getHeight()), static_cast<vImagePixelCount> (dst.getWidth()), static_cast<size_t> (dstData.lineStride) };

        // vdsp doesn't have a Planar8toBGRA function, so we just shuffle the channels manually
        // (and assume we're always little endian)
        vImageConvert_Planar8toARGB8888 (&alphaBuffer, &alphaBuffer, &alphaBuffer, &alphaBuffer, &dstBuffer, kvImageNoFlags);
        vImageOverwriteChannelsWithScalar_ARGB8888 (color.getRed(), &dstBuffer, &dstBuffer, 0x2, kvImageNoFlags);
        vImageOverwriteChannelsWithScalar_ARGB8888 (color.getGreen(), &dstBuffer, &dstBuffer, 0x4, kvImageNoFlags);
        vImageOverwriteChannelsWithScalar_ARGB8888 (color.getBlue(), &dstBuffer, &dstBuffer, 0x8, kvImageNoFlags);

        // BGRA = little endian ARGB
        vImagePremultiplyData_BGRA8888 (&dstBuffer, &dstBuffer, kvImageNoFlags);
        return dst;
    }

    [[maybe_unused]] static void tentBlurSingleChannel (juce::Image& img, unsigned int radius)
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
}
