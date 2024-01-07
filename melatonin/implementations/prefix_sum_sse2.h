#pragma once

#include <emmintrin.h>
#define LIBDIVIDE_SSE2 1
#include "../support/libdivide.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::blur
{
    typedef  __m128i v4i;
    typedef __m256i v8i;

    // this does a local prefix sum of 4 values at once
    // see https://en.algorithmica.org/hpc/algorithms/prefix/
    static inline void localPrefix (uint32_t* data)
    {
        v4i x = _mm_load_si128 ((v4i*) data); // Load data into NEON vector
        x = _mm_add_epi32(x, _mm_slli_si128(x, 4));// shift left 4 bytes, add
        x = _mm_add_epi32(x, _mm_slli_si128(x, 8)); // shift left 8 bytes, add
        _mm_store_si128 ((v4i*) data, x); // Store the result back to memory
    }

    // this broadcasts the last value of a 4-integer block to the next block
    // it's called recursively to complete the "global" prefix sum
    static inline v4i accumulate (uint32_t* currentValues, v4i previous_prefix_sum)
    {
        // Broadcast last element of block to all elements of vector
        v4i lastValueBroadcast = _mm_set1_epi32 (currentValues[3]);

        // Load the current values into lanes
        v4i currentBlock = _mm_load_si128((v4i*) currentValues);

        // Add the previous prefix sum block to current block
        v4i updatedBlock = _mm_add_epi32 (previous_prefix_sum, currentBlock);

        // Store our now updated block back to memory
        _mm_store_si128 ((v4i*) currentValues, updatedBlock);

        // Prepare the vector to be passed to the next block (because recursion)
        return _mm_add_epi32 (previous_prefix_sum, lastValueBroadcast);
    }

    // assumes buffer is always in chunks of 4
    static void prefix (uint32_t* a, int n)
    {
        // our temp block starts empty
        v4i s = _mm_setzero_si128 ();

        // n should always be divisible by 4
        jassert(n % 4 == 0);

        for (int i = 0; i < n; i += 4)
        {
            localPrefix (&a[i]);
            s = accumulate (&a[i], s);
        }

    }

    static void prefixSumSingleChannelSSE2 (juce::Image& img, unsigned int radius)
    {
        // Copy the Gin implementation for now, avoid integer division by using lookup tables
        // According to Agner Fog's Optimizing software in C++
        // lookup tables like these should be static, const, and in the function
        // this lets us multiply + right shift,  faster than integer division
        static const unsigned short stackblur_mul[255] = { 512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512, 454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312, 292, 273, 512, 482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456, 437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512, 497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328, 320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496, 485, 475, 465, 456, 446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335, 329, 323, 318, 312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512, 505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405, 399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328, 324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271, 268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456, 451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392, 388, 385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335, 332, 329, 326, 323, 320, 318, 315, 312, 310, 307, 304, 302, 299, 297, 294, 292, 289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259 };

        // shifting right divides by a power of two
        static const unsigned char stackblur_shr[255] = { 9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24 };

        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        // used for both prefixSum and double
        // this doesn't seem to suffer from being std::vector vs. std::array
        // but a static std::array<uint32_t, 1024> prefixSum seems a bit faster
        // TODO: alignment on M1 ARM doesn't seem to matter, need to test on Windows
        auto vectorSize = std::max (h, w) + 2 * radius + 2; // 2 extra px for prefix sum 0s
        vectorSize = (vectorSize + 3) & ~3; // round up to nearest multiple of 4
        std::vector<uint32_t> buffer (vectorSize);

        // this is used a lot in loops
        // it's the radius + the 2 zeros for the prefix sums
        auto imageOffset = radius + 2;

        unsigned int actualSize = w + 2 * radius + 2;

        // horizontal pass

        // pointer to the row we're working with
        auto row = data.getLinePointer (0);
        for (auto rowNumber = 0; rowNumber < h; ++rowNumber)
        {
            // copy over the row to our buffer so we can operate in-place
            // add two pixels of 0 to the start to offset for the prefix sum
            // the compiler should vectorize this
            std::copy (row, row + w, &buffer[imageOffset]);

            // left padding
            // left pixel would normally be at [radius], but it's offset by the two pixel sum 0s
            auto leftValue = buffer[imageOffset];
            // again, offset by 2 for the two pixel sum 0s
            for (unsigned int i = 2; i < imageOffset; ++i)
            {
                buffer[i] = leftValue;
            }

            // right padding
            auto rightValue = buffer[w + radius + 1];
            for (unsigned int i = w + imageOffset; i < actualSize; ++i)
            {
                buffer[i] = rightValue;
            }

            // first prefix sum
            prefix (&buffer[0], buffer.size());

            // second prefix sum
            prefix (&buffer[0], buffer.size());

            // calculate blur value for the row
            // check the #if block at bottom of file
            // In release, this faster than the SIMD route there (3.3 vs. 4.1µs on 50x50x5)
            // Presumably because it's using SIMD, magically with the shr_sum
            auto left = &buffer[0];
            auto middle = left + radius + 1;
            auto right = left + 2 * radius + 2;
            for (unsigned int i = 0; i < w; ++i)
            {
                // right + left - 2 * middle
                // [i + radius*2 + 2] + [i] + 2 * [i + radius + 1]
                // for radius 1, index 0
                // [4]    [0]    [2]
                // for radius 2, index 0
                // [6]    [0]    [3]
                auto sum = *right + *left - 2 * *middle;
                // jassert (sum / divisor <= 255);

                // this line alone adds .6µs to the 50x50x5 benchmark
                row[i] = (uint8_t) ((sum * mul_sum) >> shr_sum);
                ++left;
                ++middle;
                ++right;
            }

            // jump down a row
            row += data.lineStride;
        }
        // vertical pass
        if (h < 2)
            return;

        auto colPointer = data.getLinePointer (0);
        for (auto col = 0; col < w; ++col)
        {
            // copy over the col to our buffer so we can operate in-place
            // add two pixels of 0 to the start to offset for the prefix sum
            // the compiler should vectorize this
            auto index = radius + 2;
            for (auto i = 0; i < h; ++i)
            {
                buffer[i + index] = *(colPointer + i * data.lineStride);
            }

            // left padding
            // left pixel would normally be at [radius], but it's offset by the two pixel sum 0s
            auto leftValue = buffer[radius + 2];
            // again, offset by 2 for the two pixel sum 0s
            for (unsigned int i = 2; i < radius + 2; ++i)
            {
                buffer[i] = leftValue;
            }

            // right padding
            auto rightValue = buffer[w + radius + 1];
            for (unsigned int i = w + radius + 2; i < actualSize; ++i)
            {
                buffer[i] = rightValue;
            }

            // first prefix sum
            prefix (&buffer[0], buffer.size());

            // second prefix sum
            prefix (&buffer[0], buffer.size());

            // calculate blur value for the row
            // In release, this faster than my SIMD route below (3.3 vs. 4.1µs on 50x50x5)
            // Presumably because it's using SIMD, magically with the shr_sum
            auto left = &buffer[0];
            auto middle = left + radius + 1;
            auto right = left + 2 * radius + 2;
            for (unsigned int i = 0; i < w; ++i)
            {
                // right + left - 2 * middle
                // [i + radius*2 + 2] + [i] + 2 * [i + radius + 1]
                // for radius 1, index 0
                // [4]    [0]    [2]
                // for radius 2, index 0
                // [6]    [0]    [3]
                const auto sum = *right + *left - 2 * *middle;
                // jassert (sum / divisor <= 255);

                // this line alone adds .6µs to the 50x50x5 benchmark
                // TODO: faster to place in yet another temp buffer?
                *(colPointer + i * data.lineStride) = (uint8_t) ((sum * mul_sum) >> shr_sum);
                ++left;
                ++middle;
                ++right;
            }

            // makes a big difference to performance to use a pointer here
            colPointer += data.pixelStride;
        }
    }
}

#if false
// shr isn't happy on SIMD, so we're using libdivide
// libdivide requires a special constructor
//            auto divisor = libdivide::libdivide_u32_gen ((radius + 1) * (radius + 1));
//
//            // Load the constant '2' into a NEON vector
//            v4i two = vdupq_n_u32 (2);
//
//            // set up the pointers to the resultant prefix sums
//            auto left = &buffer[0];
//            auto middle = left + radius + 1;
//            auto right = left + 2 * radius + 2;
//
//            for (unsigned int i = 0; i < w; i += 4)
//            {
//                // Process four pixels at a time
//                // Load the values into NEON vectors
//                v4i v_left = vld1q_u32 (left);
//                v4i v_middle = vld1q_u32 (middle);
//                v4i v_right = vld1q_u32 (right);
//
//                // Perform the arithmetic
//                v4i sum = vaddq_u32 (v_right, v_left);
//                sum = vmlsq_u32 (sum, two, v_middle); // Equivalent to sum - 2 * middle
//
//                // Assuming mul_sum and shr_sum can be applied without overflow
//                // Multiply and shift, this isn't any faster using the branchless one
//                sum = libdivide::libdivide_u32_do_vec128 (sum, &divisor);
//
//                // Store the result back to the row
//                uint16x4_t narrow_values = vmovn_u32 (sum); // Narrow from 32-bit to 16-bit
//                uint8x8_t result = vreinterpret_u8_u16 (vmovn_u16 (vcombine_u16 (narrow_values, narrow_values))); // Narrow from 16-bit to 8-bit
//                vst1_u8 (&row[i], result); // Store the result
//
//                // Increment pointers
//                left += 4;
//                middle += 4;
//                right += 4;
//            }

#endif
