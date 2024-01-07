#pragma once

#include "juce_audio_basics/juce_audio_basics.h" // JUCE_USE_ARM_NEON
#include "juce_graphics/juce_graphics.h"

#if JUCE_MAC || JUCE_IOS
    #include <Accelerate/Accelerate.h>
#endif

#if JUCE_USE_ARM_NEON
    #include <arm_neon.h>
#endif

namespace melatonin
{

#if JUCE_USE_ARM_NEON

    // MxN (num rows x num cols) -> NxN
    // TODO: needs to take a JUCE image and deal with linestride
    static void neonClockwise (const uint8_t* src, uint8_t* dst, int M, int N)
    {
        // go through each col
        for (int col = 0; col < N; ++col)
        {
            // each row
            // TODO: M has to be a multiple of 16, write code to handle the remainder
            for (int row = 0; row < M; row += 16)
            {
                // Load 16 values from the column
                uint8x16_t data = vld1q_u8 (&src[(M - row - 16) * N + col]);
                // Store 16 bytes into the row
                vst1q_u8 (&dst[col * M + row], data);
            }
        }
    }

    static void neonCounterClockwise (const uint8_t* src, uint8_t* dst, int M, int N)
    {
        for (int col = 0; col < N; ++col)
        {
            for (int row = 0; row < M; row += 16)
            {
                // Load 16 bytes from the column
                uint8x16_t data = vld1q_u8 (&src[row * N + (N - col - 1)]);
                // Store 16 bytes into the row
                vst1q_u8 (&dst[(N - col - 1) * M + row], data);
            }
        }
    }
#endif

    static void rotateSingleChannel (juce::Image& src, uint8_t* dst)
    {
        // TODO: Check dst dimensions to ensure they don't exceed the buffer size

        auto M = src.getWidth(); // num rows, aka width
        auto N = src.getHeight(); // num cols, aka height
        auto data = juce::Image::BitmapData (src, juce::Image::BitmapData::readOnly);
        auto lineStride = data.lineStride;
#if JUCE_MAC || JUCE_IOS

        // vImageBuffrers take HEIGHT first, then width!
        vImage_Buffer srcBuffer { data.getLinePointer (0), (vImagePixelCount) N, (vImagePixelCount) M, (vImagePixelCount) lineStride };

        // our "linestride" for the destination is just our height (no padding)
        vImage_Buffer dstBuffer { (void*) dst, (vImagePixelCount) M, (vImagePixelCount) N, (vImagePixelCount) N };
        vImageRotate90_Planar8 (&srcBuffer, &dstBuffer, kRotate90DegreesClockwise, 0, kvImageNoFlags);
#elif JUCE_USE_ARM_NEON
        neonClockwise (src, dst, M, N);
#endif
    }

    static void unrotateSingleChannel (uint8_t* src, juce::Image& dst)
    {
        // TODO: Check dst dimensions to ensure they don't exceed the buffer size

        auto M = dst.getWidth(); // MxN is the dst dimensions, so this is *height* of src
        auto N = dst.getHeight(); // num rows / width / linestride of previously rotated, HEIGHT of juce image
        auto data = juce::Image::BitmapData (dst, juce::Image::BitmapData::readOnly);
        auto lineStride = data.lineStride;
#if JUCE_MAC || JUCE_IOS
        vImage_Buffer srcBuffer { (void*) src, (vImagePixelCount) M, (vImagePixelCount) N, (vImagePixelCount) N };
        vImage_Buffer dstBuffer { data.getLinePointer (0), (vImagePixelCount) N, (vImagePixelCount) M, (vImagePixelCount) lineStride };
        vImageRotate90_Planar8 (&srcBuffer, &dstBuffer, kRotate90DegreesCounterClockwise, 0, kvImageNoFlags);
#elif JUCE_USE_ARM_NEON
        neonCounterClockwise (src, dst, M, N);
#endif
    }
}
