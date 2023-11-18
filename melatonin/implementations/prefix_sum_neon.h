#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include <arm_neon.h>

namespace melatonin::blur
{
    typedef uint32x4_t v4i;

    // this does a local prefix sum of 4 values at once
    // see https://en.algorithmica.org/hpc/algorithms/prefix/
    static inline void localPrefix (uint32_t* data)
    {
        v4i x = vld1q_u32 (data); // Load data into NEON vector
        uint32x4_t zero = vdupq_n_u32 (0);
        x = vaddq_u32 (x, vextq_u32 (zero, x, 3)); // shift one lane, add
        x = vaddq_u32 (x, vextq_u32 (zero, x, 2)); // shift one lane, add
        vst1q_u32 (data, x); // Store the result back to memory
    }

    // this broadcasts the last value of a 4-integer block to the next block
    // it's called recursively to complete the "global" prefix sum
    static inline v4i accumulate (uint32_t* currentValues, v4i previous_prefix_sum)
    {
        // Broadcast last element of block to all elements of vector
        v4i lastValueBroadcast = vdupq_n_u32 (currentValues[3]);

        // Load the current values into lanes
        v4i currentBlock = vld1q_u32 (currentValues);

        // Add the previous prefix sum block to current block
        v4i updatedBlock = vaddq_u32 (previous_prefix_sum, currentBlock);

        // Store our now updated block back to memory
        vst1q_u32 (currentValues, updatedBlock);

        // Prepare the vector to be passed to the next block (because recursion)
        return vaddq_u32 (previous_prefix_sum, lastValueBroadcast);
    }

    // assumes buffer is always in chunks of 4
    static void prefix (uint32_t* a, int n)
    {
        for (int i = 0; i < n; i += 4)
            localPrefix (&a[i]);

        // our temp block starts empty
        v4i s = vdupq_n_u32 (0);

        for (int i = 0; i < n; i += 4)
            s = accumulate (&a[i], s);
    }

    static void prefixSumSingleChannelNeon (juce::Image& img, unsigned int radius)
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

        auto divisor = (radius + 1) * (radius + 1);
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        // used for both prefixSum and double
        // this doesn't seem to suffer from being std::vector vs. std::array
        // but a static std::array<uint32_t, 1024> prefixSum seems a bit faster
        // TODO: alignment on M1 ARM doesn't seem to matter, need to test on Windows
        auto vectorSize = w + 2 * radius + 2;
        vectorSize = (vectorSize + 3) & ~3; // round up to nearest multiple of 4
        std::vector<uint32_t> buffer (vectorSize);

        unsigned int actualSize = w + 2 * radius + 2;

        // horizontal pass
        for (auto rowNumber = 0; rowNumber < h; ++rowNumber)
        {
            // pointer to the row we're working with
            auto row = data.getLinePointer (rowNumber);

            // copy over the row to our buffer so we can operate in-place
            std::copy (row, row + w, buffer.begin() + radius);

            // left padding
            auto leftValue = buffer[radius + 1];
            for (unsigned int i = 0; i < radius; ++i)
            {
                buffer[i] = leftValue;
            }

            // right padding
            auto rightValue = buffer[w + radius];
            for (unsigned int i = w + radius; i < actualSize; ++i)
            {
                buffer[i] = rightValue;
            }

            prefix (&buffer[0], vectorSize);

            // second prefix sum
            prefix (&buffer[0], vectorSize);

            // calculate blur value for the row
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
                jassert (sum / divisor <= 255);
                row[i] = (uint8_t) ((sum * mul_sum) >> shr_sum);
                ++left;
                ++middle;
                ++right;
            }
        }
    }

}
