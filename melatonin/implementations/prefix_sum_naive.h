#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::blur
{
    // like the Gin implementation, avoid integer division by using lookup tables
    // this lets us multiply + right shift, much faster than integer division
    const unsigned short stackblur_mul[255] = { 512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512, 454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312, 292, 273, 512, 482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456, 437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512, 497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328, 320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496, 485, 475, 465, 456, 446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335, 329, 323, 318, 312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512, 505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405, 399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328, 324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271, 268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456, 451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392, 388, 385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335, 332, 329, 326, 323, 320, 318, 315, 312, 310, 307, 304, 302, 299, 297, 294, 292, 289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259 };

    // shifting right divides by a power of two
    const unsigned char stackblur_shr[255] = { 9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24 };

    static void prefixSumSingleChannel (juce::Image& img, unsigned int radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto divisor = (radius + 1) * (radius + 1);
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        // used for both prefixSum and double
        std::vector<uint32_t> prefixSum (w + 2 * radius + 2);

        unsigned int actualSize = w + 2 * radius + 2;

        auto row = data.getLinePointer (0);

        // horizontal pass
        for (auto rowNumber = 0; rowNumber < h; ++rowNumber)
        {
            // prefix sum (exclusive, with extra 0 at the start)
            prefixSum[0] = 0;

            // left padding
            for (unsigned int i = 1; i < radius + 1; ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[0];
            }

            // calculate prefix sum for the row, with padding
            for (unsigned int i = radius + 1; i < w + radius; ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[i - radius - 1];
            }

            // right padding
            for (unsigned int i = w + radius; i < actualSize; ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[w - 1];
            }

            uint32_t lastPrefix = 0;
            uint32_t nextPrefix = 0;
            // calculate 2nd order prefix sum for the row in-place!
            for (unsigned int i = 2; i < actualSize; ++i)
            {
                // stash the current prefixSum, we'll need it next iteration
                nextPrefix = prefixSum[i];

                // overwrite with the secondPrefix
                prefixSum[i] = prefixSum[i-1] + lastPrefix;
                lastPrefix = nextPrefix;
            }
            prefixSum[1] = 0;

            // calculate blur value for the row
            for (unsigned int i = 0; i < w; ++i)
            {
                // right + left - 2 * middle
                // [i + radius*2 + 2] + [i] + 2 * [i + radius + 1]
                // for radius 1, index 0
                // [4]    [0]    [2]
                // for radius 2, index 0
                // [6]    [0]    [3]
                auto sum = prefixSum[i + 2 * radius + 2] + prefixSum[i] - 2 * (prefixSum[i + radius + 1]);
                jassert (sum / divisor <= 255);
                row[i] = (uint8_t) ((sum * mul_sum) >> shr_sum);
            }

            row += rowNumber * data.lineStride;
        }
    }

    static void prefixSumSingleChannelNaive (juce::Image& img, unsigned int radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto divisor = (radius + 1) * (radius + 1);
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];
        // initialization

        // prefixSum[n] is a cumulative sum of all elements row[0] to row[n-1]
        // it has n + 2r + 1 elements (to account for padding to edges)
        // TODO: benchmark padding vs. variable divisor (which has improved edge bleed)
        std::vector<uint32_t> prefixSum (w + 2 * radius + 1);

        // secondOrderPrefixSum[n] is a cumulative sum of all elements prefixSum[0] to prefixSum[n-1]
        // it has n + 2r + 2 elements
        std::vector<uint32_t> secondOrderPrefixSum (w + 2 * radius + 2);

        auto row = data.getLinePointer (0);

        // horizontal pass
        for (auto rowNumber = 0; rowNumber < h; ++rowNumber)
        {
            // exclusive prefix sum
            prefixSum[0] = 0;
            secondOrderPrefixSum[0] = 0;
            secondOrderPrefixSum[1] = 0;

            // left padding
            for (unsigned int i = 1; i < radius + 1; ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[0];
            }

            // calculate prefix sum for the row, with padding
            for (unsigned int i = radius + 1; i < w + radius; ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[i - radius - 1];
            }

            // right padding
            for (unsigned int i = w + radius; i < prefixSum.size(); ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[w - 1];
            }

            // calculate 2nd order prefix sum for the row
            for (unsigned int i = 2; i < secondOrderPrefixSum.size(); ++i)
            {
                secondOrderPrefixSum[i] = secondOrderPrefixSum[i - 1] + prefixSum[i - 1];
            }

            // calculate blur value for the row
            for (unsigned int i = 0; i < w; ++i)
            {
                // right + left - 2 * middle
                // [i + radius*2 + 2] + [i] + 2 * [i + radius + 1]
                // for radius 1, index 0
                // [4]    [0]    [2]
                // for radius 2, index 0
                // [6]    [0]    [3]
                auto sum = secondOrderPrefixSum[i + 2 * radius + 2] + secondOrderPrefixSum[i] - 2 * (secondOrderPrefixSum[i + radius + 1]);
                jassert (sum / divisor <= 255);
                row[i] = (uint8_t) ((sum / divisor));
            }

            row += rowNumber * data.lineStride;
        }
    }

#if false
    // this version maintains 2 "diameters" worth of prefix sums
    static void prefixSumSingleChannel (juce::Image& img, unsigned int radius)
    {
        const auto w = (unsigned int) img.getWidth();
        const auto h = (unsigned int) img.getHeight();

        juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);

        auto divisor = (radius + 1) * (radius + 1);
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        // initialization
        auto row = data.getLinePointer (0);

        // exclusive prefix sum
        constexpr auto maxDiameter = 10;
        uint32_t prefixSum = 0;
        std::array<uint32_t, maxDiameter> secondOrderPrefixSum {};

        // calculate the left padding
        for (unsigned int i = 1; i < radius + 1; ++i)
        {
            prefixSum += row[0];
            secondOrderPrefixSum[i] = prefixSum;
        }

        // horizontal pass
        for (auto rowNumber = 0; rowNumber < h; ++rowNumber)
        {
            for (auto i = 0; i < w; ++i)
            {
                prefixSum += row[i];
                secondOrderPrefixSum += prefixSum;
                auto sum = secondOrderPrefixSum[i + 2 * radius + 2] + secondOrderPrefixSum[i] - 2 * (secondOrderPrefixSum[i + radius + 1]);
                row[i] = (uint8_t) ((sum * mul_sum) >> shr_sum);
            }

            // right padding
            for (unsigned int i = w + radius; i < prefixSum.size(); ++i)
            {
                prefixSum[i] = prefixSum[i - 1] + row[w - 1];
            }

            // calculate 2nd order prefix sum for the row
            for (unsigned int i = 2; i < secondOrderPrefixSum.size(); ++i)
            {
                secondOrderPrefixSum[i] = secondOrderPrefixSum[i - 1] + prefixSum[i - 1];
            }

            // calculate blur value for the row
            for (unsigned int i = 0; i < w; ++i)
            {
                // right + left - 2 * middle
                // [i + radius*2 + 2] + [i] + 2 * [i + radius + 1]
                // for radius 1, index 0
                // [4]    [0]    [2]
                // for radius 2, index 0
                // [6]    [0]    [3]
                auto sum = secondOrderPrefixSum[i + 2 * radius + 2] + secondOrderPrefixSum[i] - 2 * (secondOrderPrefixSum[i + radius + 1]);
                jassert (sum / divisor <= 255);
                row[i] = (uint8_t) ((sum / divisor));
            }

            row += rowNumber * data.lineStride;
        }
    }
#endif
}
