// #include "../melatonin/implementations/dequeue.h"
#include "../melatonin/implementations/platform_includes.h"

#include "../melatonin/implementations/float_vector_stack_blur.h"
#include "../melatonin/implementations/gin.h"
// #include "../melatonin/implementations/naive.h"
// #include "../melatonin/implementations/naive_class.h"
// #include "../melatonin/implementations/naive_with_martin_optimization.h"
// #include "../melatonin/implementations/templated_function.h"
#include "../melatonin/internal/implementations.h"

// These require melatonin::vector, not in this repo
// #include "../melatonin/implementations/templated_function_float.h"
// #include "vector.h"
// #include "vector_class.h"
// #include "vector_optimized.h"

#include "../melatonin_blur.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <juce_graphics/juce_graphics.h>

// Keeps the actual tests DRY
// bit ugly to wrap everything in std::function
// it's because our stack blur is a class, not free func
using BlurFunction = std::function<void (juce::Image&, size_t)>;
inline auto singleColorBlurImplementation()
{
    return GENERATE (
       // std::make_pair ("gin", BlurFunction { [] (juce::Image& img, size_t radius) { melatonin::stackBlur::ginSingleChannel (img, (unsigned int) radius); } }),
        // std::make_pair ("prefix sum naive", BlurFunction { [] (juce::Image& img, int radius) { melatonin::blur::prefixSumSingleChannel (img, radius); } }),
        // std::make_pair ("dequeue", BlurFunction { [] (juce::Image& img, int radius) { melatonin::stackBlur::dequeueSingleChannel (img, radius); } }),
        // std::make_pair ("circularBuffer", BlurFunction { [] (juce::Image& img, int radius) { melatonin::stackBlur::circularBufferSingleChannel (img, radius); } }),
        // std::make_pair ("martin optimization", BlurFunction { [] (juce::Image& img, int radius) { melatonin::stackBlur::martinOptimizationSingleChannel (img, radius); } }),
        //    std::make_pair ("vector", BlurFunction { [] (juce::Image& img, int radius) { melatonin::stackBlur::vectorSingleChannel (img, radius); } }),
        //    std::make_pair ("vector optimized", BlurFunction { [] (juce::Image& img, int radius) { melatonin::stackBlur::vectorOptimizedSingleChannel (img, radius); } }),
        //    std::make_pair ("vector class", BlurFunction { [&] (juce::Image& img, int radius) { melatonin::VectorStackBlur stackBlur (img, radius); } }),
        //std::make_pair ("juce's FloatVectorOperations", BlurFunction { [&] (juce::Image& img, size_t radius) { melatonin::blur::juceFloatVectorSingleChannel (img, radius); } }),
        // std::make_pair ("naive class", BlurFunction { [&] (juce::Image& img, int radius) { melatonin::NaiveStackBlur stackBlur (img, radius); } }),
        // std::make_pair ("templated function", BlurFunction { [&] (juce::Image& img, int radius) { melatonin::stackBlur::singleChannelTemplated (img, radius); } }),
        //         std::make_pair ("templated function float", BlurFunction { [&] (juce::Image& img, int radius) { melatonin::stackBlur::templatedFloatSingleChannel (img, radius); } }),
        std::make_pair ("Direct2D", BlurFunction { [&] (juce::Image& img, size_t radius) { melatonin::blur::direct2DSingleChannel (img, radius); } }));
        //std::make_pair ("Melatonin", BlurFunction { [&] (juce::Image& img, size_t radius) { melatonin::blur::singleChannel (img, radius); } }));
}

inline auto rgbaBlurImplementation()
{
    return GENERATE (
        std::make_pair ("gin", BlurFunction { [] (juce::Image& img, size_t radius) { melatonin::stackBlur::ginARGB (img, (unsigned int) radius); } }),
        //std::make_pair ("juce's FloatVectorOperations", BlurFunction { [&] (juce::Image& img, size_t radius) { melatonin::blur::juceFloatVectorARGB (img, radius); } }),
        std::make_pair ("Melatonin", BlurFunction { [&] (juce::Image& img, size_t radius) {
            // argb goes haywire in-place, so we need to copy
            auto src = img.createCopy();
            melatonin::blur::argb (src, img, radius);
        } }));
}

/*
 * TODO: I made the dubious decision to test with float values
 * Probably should use uint8 for clarity, as they are more discrete
 * and therefore easier to test.
 */
TEST_CASE ("Melatonin Blur")
{
    /*
     * One big advantage of the blur occuring in the two passes is that to a large extent
     * we can test the horizontal and vertical passes seperetely,
     * by having the test image be either a single horizontal row or a vertical column.
     */
    SECTION ("single channel horizontal pass radius 1")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 10, 1, true);

        SECTION ("all bright")
        {
            // These calls actually have to be in the section for GENERATE to work
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // fill image with 1.0
                for (auto x = 0; x < image.getWidth(); ++x)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                }
            }
            std::vector<float> expected = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.00001f));
            }
        }

        SECTION ("center 2 bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                data.setPixelColour (4, 0, juce::Colours::black);
                data.setPixelColour (5, 0, juce::Colours::black);
            }

            std::vector<float> expected = { 0.0f, 0.0f, 0.0f, 0.24706f, 0.74902f, 0.74902f, 0.24706f, 0.0f, 0.0f, 0.0f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.004f));
            }
        }

        SECTION ("first and last pixel bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                data.setPixelColour (0, 0, juce::Colours::black);
                data.setPixelColour (9, 0, juce::Colours::black);
            }

            std::vector<float> expected = { 0.74902f, 0.24706f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.24706f, 0.74902f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.004f));
            }
        }
    }

    SECTION ("single channel horizontal pass radius 2")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 10, 1, true);

        SECTION ("all bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // fill image with 1.0
                for (auto x = 0; x < image.getWidth(); ++x)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                }
            }

            std::vector<float> expected = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.00001f));
            }
        }

        SECTION ("center 2 bright has symmetrical result")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                data.setPixelColour (4, 0, juce::Colours::black);
                data.setPixelColour (5, 0, juce::Colours::black);
            }

            std::vector<float> expected = { 0.0f, 0.0f, 0.1098f, 0.33333f, 0.55294f, 0.55294f, 0.33333f, 0.1098f, 0.0f, 0.0f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.004f));
            }
        }

        SECTION ("first and last pixel bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                data.setPixelColour (0, 0, juce::Colours::black);
                data.setPixelColour (9, 0, juce::Colours::black);
            }

            std::vector<float> expected = { 0.66667f, 0.33333f, 0.1098f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1098f, 0.33333f, 0.66667f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.00001f));
            }
        }

        SECTION ("every other bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();
            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                for (auto x = 0; x < image.getWidth(); x += 2)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                }
            }

            std::vector<float> expected = { 0.77647f, 0.55294f, 0.55294f, 0.44314f, 0.55294f, 0.44314f, 0.55294f, 0.44314f, 0.44314f, 0.21961f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.004f));
            }
        }

        SECTION ("every 3rd bright")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                for (auto x = 0; x < image.getWidth(); x += 3)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                }
            }

            std::vector<float> expected = { 0.66667f, 0.44314f, 0.33333f, 0.33333f, 0.33333f, 0.33333f, 0.33333f, 0.33333f, 0.44314f, 0.66667f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelRow (image, 0), Catch::Matchers::Approx (expected).margin (0.00001f));
            }
        }
    }

    SECTION ("single channel vertical pass")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 1, 10, true);

        SECTION ("all bright radius 2")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // fill image with 1.0
                for (auto y = 0; y < image.getHeight(); ++y)
                {
                    data.setPixelColour (0, y, juce::Colours::black);
                }
            }

            std::vector<float> expected = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 2);
                REQUIRE_THAT (pixelCol (image, 0), Catch::Matchers::Approx (expected).margin (0.00001f));
            }
        }

        SECTION ("center 2 bright has symmetrical result radius 1")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                data.setPixelColour (0, 4, juce::Colours::black);
                data.setPixelColour (0, 5, juce::Colours::black);
            }

            // the vertical pass compounds on the horizontal, so this looks different from the horizontal pass result of
            //  { 0.0f, 0.0f, 0.1098f, 0.33333f, 0.55294f, 0.55294f, 0.33333f, 0.1098f, 0.0f, 0.0f }
            std::vector<float> expected = { 0.0f, 0.0f, 0.0f, 0.24706f, 0.74902f, 0.74902f, 0.24706f, 0.0f, 0.0f, 0.0f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                REQUIRE_THAT (pixelCol (image, 0), Catch::Matchers::Approx (expected).margin (0.004f));
            }
        }
    }

    SECTION ("Happy in 2D")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 10, 10, true);

        std::vector<float> edge = { 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f };
        std::vector<float> nextToEdge = { 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f, 0.24706f };
        std::vector<float> empty = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        SECTION ("top edge outlined (radius 1)")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // fill image with 1.0
                for (auto x = 0; x < image.getHeight(); ++x)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                }
            }

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                CHECK_THAT (pixelRow (image, 0), Catch::Matchers::Approx (edge).margin (0.004f));
                CHECK_THAT (pixelRow (image, 1), Catch::Matchers::Approx (nextToEdge).margin (0.004f));
                for (auto y = 2; y < image.getHeight(); ++y)
                {
                    CHECK_THAT (pixelRow (image, y), Catch::Matchers::Approx (empty).margin (0.00001f));
                }
            }
        }

        SECTION ("left edge outlined (radius 1)")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // left edge at 1.0
                for (auto y = 0; y < image.getHeight(); ++y)
                {
                    data.setPixelColour (0, y, juce::Colours::black);
                }
            }

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                CHECK_THAT (pixelCol (image, 0), Catch::Matchers::Approx (edge).margin (0.004f));
                CHECK_THAT (pixelCol (image, 1), Catch::Matchers::Approx (nextToEdge).margin (0.004f));
                for (auto y = 2; y < image.getHeight(); ++y)
                {
                    CHECK_THAT (pixelCol (image, y), Catch::Matchers::Approx (empty).margin (0.00001f));
                }
            }
        }

        SECTION ("right edge outlined")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // right edge at 1.0
                for (auto y = 0; y < image.getHeight(); ++y)
                {
                    data.setPixelColour (image.getWidth() - 1, y, juce::Colours::black);
                }
            }

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                CHECK_THAT (pixelCol (image, image.getWidth() - 1), Catch::Matchers::Approx (edge).margin (0.004f));
                CHECK_THAT (pixelCol (image, image.getWidth() - 2), Catch::Matchers::Approx (nextToEdge).margin (0.004f));
                for (auto y = 0; y < image.getHeight() - 2; ++y)
                {
                    CHECK_THAT (pixelCol (image, y), Catch::Matchers::Approx (empty).margin (0.00001f));
                }
            }
        }

        SECTION ("bottom edge outlined")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // bottom edge at 1.0
                for (auto x = 0; x < image.getWidth(); ++x)
                {
                    data.setPixelColour (x, image.getHeight() - 1, juce::Colours::black);
                }
            }

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);
                CHECK_THAT (pixelRow (image, image.getHeight() - 1), Catch::Matchers::Approx (edge).margin (0.004f));
                CHECK_THAT (pixelRow (image, image.getHeight() - 2), Catch::Matchers::Approx (nextToEdge).margin (0.004f));
                for (auto y = 0; y < image.getHeight() - 2; ++y)
                {
                    CHECK_THAT (pixelRow (image, y), Catch::Matchers::Approx (empty).margin (0.00001f));
                }
            }
        }

        SECTION ("ALL outer edges outlined")
        {
            const auto& [name, blur] = singleColorBlurImplementation();

            {
                juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                // outline with 1.0
                for (auto y = 0; y < image.getHeight(); ++y)
                {
                    data.setPixelColour (0, y, juce::Colours::black);
                    data.setPixelColour (image.getWidth() - 1, y, juce::Colours::black);
                }

                for (auto x = 0; x < image.getWidth(); ++x)
                {
                    data.setPixelColour (x, 0, juce::Colours::black);
                    data.setPixelColour (x, image.getHeight() - 1, juce::Colours::black);
                }
            }

            std::vector<float> corners = { 0.93725f, 0.81176f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.74902f, 0.81176f, 0.93725f };
            std::vector<float> middle = { 0.74902f, 0.24706f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.24706f, 0.74902f };

            DYNAMIC_SECTION (name)
            {
                blur (image, 1);

                // all outside edges
                CHECK_THAT (pixelCol (image, 0), Catch::Matchers::Approx (corners).margin (0.004f));
                CHECK_THAT (pixelRow (image, 0), Catch::Matchers::Approx (corners).margin (0.004f));
                CHECK_THAT (pixelCol (image, image.getWidth() - 1), Catch::Matchers::Approx (corners).margin (0.004f));
                CHECK_THAT (pixelRow (image, image.getHeight() - 1), Catch::Matchers::Approx (corners).margin (0.004f));

                // inner rows
                for (auto dimension = 2; dimension < image.getHeight() - 2; ++dimension)
                {
                    CHECK_THAT (pixelCol (image, dimension), Catch::Matchers::Approx (middle).margin (0.004f));
                }
            }
        }
    }

    /*
     * The only thing to take into consideration with ARGB and blurring alpha is that JUCE will report
     * as if *only* the alpha was blurred, but in reality, the data in the pixels are premultiplied.
     *
     * For example, toDisplayString will return results like the following
     *
     * Before:
     *   Image: 10x1
     *   00000000, 00000000, 00000000, 00000000, FFFFFFFF, FFFFFFFF, 00000000, 00000000, 00000000, 00000000
     * After:
     *   Image: 10x1
     *   00000000, 00000000, 1CFFFFFF, 55FFFFFF, 8DFFFFFF, 8DFFFFFF, 55FFFFFF, 1CFFFFFF, 00000000, 00000000
     *
     *   Although this hints at some interesting potential optimizations for solid color areas
     *   it doesn't make our testing life easy.
     *
     *   (This is why we use ActualPixel behind the scenes in the test helpers)
     */

    SECTION ("ARGB")
    {
        SECTION ("Horizontal Pass (10x1 image size)")
        {
            juce::Image image (juce::Image::PixelFormat::ARGB, 10, 1, true);

            SECTION ("red 1.0, green 1.0, blue 1.0")
            {
                const auto& [name, blur] = rgbaBlurImplementation();

                {
                    juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                    // fill image with 1.0
                    for (auto x = 0; x < image.getWidth(); ++x)
                    {
                        data.setPixelColour (x, 0, juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 1.0f));
                    }
                }

                //  underlying colors are 8-bit (0-255) so translation back to float is messy
                std::vector<float> expected = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

                DYNAMIC_SECTION (name)
                {
                    for (auto i = 0; i < 3; ++i)
                    {
                        DYNAMIC_SECTION ("channel " << i << " at 1.0")
                        {
                            // make sure we set the pixels correctly to begin with
                            REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (expected).margin (0.00001f));

                            blur (image, 1);
                            REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (expected).margin (0.00001f));
                        }
                    }
                }
            }

            SECTION ("channels are seperated (one chan at 1.0)")
            {
                for (auto i = 0; i < 3; ++i)
                {
                    DYNAMIC_SECTION ("only channel " << i << " at max ")
                    {
                        // has to be in the section for GENERATE to work
                        const auto& [name, blur] = rgbaBlurImplementation();

                        // Alpha, the "last" component of BGRA in little endian must be 1.0
                        // JUCE uses premultiplied alpha pixels
                        // This took me 1-2 hours to realize
                        std::vector<float> pixel = { 0.0f, 0.0f, 0.0f, 1.0f };

                        // modify just one channel of the pixel
                        pixel[(size_t) i] = 1.0f;

                        std::vector<float> expected ((size_t) image.getWidth());

                        {
                            juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                            // add copies of this pixel to the image and expected
                            for (auto x = 0; x < image.getWidth(); ++x)
                            {
                                // yeah, this is sort of a mindfuck, going from BGRA -> RGBA
                                auto pixelColor = juce::Colour::fromFloatRGBA (pixel[2], pixel[1], pixel[0], pixel[3]);
                                data.setPixelColour (x, 0, pixelColor);
                                expected[(size_t) x] = pixel[(size_t) i];
                            }
                        }

                        // make sure we set the pixels correctly to begin with
                        REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (expected).margin (0.00001f));

                        DYNAMIC_SECTION (name)
                        {
                            blur (image, 1);
                            REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (expected).margin (0.00001f));
                        }
                    }
                }
            }
            SECTION ("center 2 bright is symmetrical in each channel")
            {
                const auto& [name, blur] = rgbaBlurImplementation();

                {
                    juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

                    data.setPixelColour (4, 0, juce::Colours::white);
                    data.setPixelColour (5, 0, juce::Colours::white);
                }

                // initial state
                std::vector<float> initial = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

                // identical to single channel blur's horizontal pass
                std::vector<float> expected = { 0.0f, 0.0f, 0.1098f, 0.33333f, 0.55294f, 0.55294f, 0.33333f, 0.1098f, 0.0f, 0.0f };

                DYNAMIC_SECTION (name)
                {
                    for (auto i = 0; i < 3; ++i)
                    {
                        DYNAMIC_SECTION ("channel " << i << " of little endian bgra")
                        {
                            // sanity check our pixelRow helper before the blur
                            REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (initial).margin (0.004f));
                            blur (image, 2);
                            REQUIRE_THAT (pixelRow (image, 0, i), Catch::Matchers::Approx (expected).margin (0.004f));
                        }
                    }
                }
            }
        }
        SECTION ("Vertical Pass (1x10 image size)")
        {
            juce::Image image (juce::Image::PixelFormat::ARGB, 1, 10, true);

            SECTION ("center 2 bright is symmetrical in each channel")
            {
                const auto& [name, blur] = rgbaBlurImplementation();

                {
                    juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);
                    data.setPixelColour (0, 4, juce::Colours::white);
                    data.setPixelColour (0, 5, juce::Colours::white);
                }
                // initial state
                std::vector<float> initial = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

                // identical to single channel blur's *vertical* pass (differs from horizontal)
                std::vector<float> expected = { 0.0f, 0.0f, 0.1098f, 0.33333f, 0.55294f, 0.55294f, 0.33333f, 0.1098f, 0.0f, 0.0f };

                DYNAMIC_SECTION (name)
                {
                    for (auto i = 0; i < 3; ++i)
                    {
                        DYNAMIC_SECTION ("channel " << i << " of little endian bgra")
                        {
                            // sanity check our pixelRow helper and state before the blur
                            REQUIRE_THAT (pixelCol (image, 0, i), Catch::Matchers::Approx (initial).margin (0.004f));
                            blur (image, 2);
                            REQUIRE_THAT (pixelCol (image, 0, i), Catch::Matchers::Approx (expected).margin (0.004f));
                        }
                    }
                }
            }
        }
    }
}
