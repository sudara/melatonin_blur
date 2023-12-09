#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Drop Shadow")
{
    // here's what our test image looks like:
    // 0=white, 1=black, just to be annoying...

    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  1  1  1  0  0  0
    // 0  0  0  1  1  1  0  0  0
    // 0  0  0  1  1  1  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0

    // create a 3px by 3px square
    auto bounds = juce::Rectangle<float> (3, 3);
    juce::Path p;

    // stick the 3x3 box centered inside a 9x9
    p.addRectangle (bounds.translated (3, 3));

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);

    SECTION ("no shadow")
    {
        g.fillAll (juce::Colours::white);

        g.setColour (juce::Colours::black);
        g.fillPath (p);

        // nothing should be outside the top left corner
        REQUIRE (result.getPixelAt (2, 2).toDisplayString (true) == "FFFFFFFF"); // ARGB

        // starting at pixel 4,4 we have black
        REQUIRE (result.getPixelAt (4, 4).toDisplayString (true) == "FF000000");
    }

    SECTION ("default constructor")
    {
        g.fillAll (juce::Colours::white);
        melatonin::DropShadow shadow;
        shadow.render (g, p);
        CHECK(filledBounds (result) == juce::Rectangle<int> (3, 3, 3, 3));

        save_test_image(result, "default constructor");
        shadow.setRadius(5);
        shadow.render (g, p);
        CHECK(filledBounds (result) == juce::Rectangle<int> (0, 0, 9, 9));
    }

    SECTION ("single shadow")
    {
        g.fillAll (juce::Colours::white);
        melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };
        shadow.render (g, p);
        g.setColour (juce::Colours::black);
        g.fillPath (p);
        save_test_image (result, "single_shadow");

        // TODO: I'd like to reduce the margin on these tests
        // Right now they are within 2 hex values of the expected value
        // It seems that windows itself may be compositing slightly differently?
        SECTION ("left edge")
        {
            CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0)); // completely white
            CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91372549).margin (0.01)); // 1st px blur
            CHECK (result.getPixelAt (2, 4).getBrightness() == Catch::Approx (0.741176471).margin (0.01)); // 2nd px blur
            CHECK (result.getPixelAt (3, 4).getBrightness() == Catch::Approx (0.0));
        }

        SECTION ("top edge")
        {
            CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0));
            CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91372549).margin (0.01));
            CHECK (result.getPixelAt (4, 2).getBrightness() == Catch::Approx (0.741176471).margin (0.01));
            CHECK (result.getPixelAt (4, 3).getBrightness() == Catch::Approx (0.0));
        }

        SECTION ("right edge")
        {
            CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0));
            CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91372549).margin (0.01));
            CHECK (result.getPixelAt (6, 4).getBrightness() == Catch::Approx (0.741176471).margin (0.01));
            CHECK (result.getPixelAt (5, 4).getBrightness() == Catch::Approx (0.0));
        }

        SECTION ("bottom edge")
        {
            // check the bottom edge, 1 pixel into the left
            CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0));
            CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91372549).margin (0.01));
            CHECK (result.getPixelAt (4, 6).getBrightness() == Catch::Approx (0.741176471).margin (0.01));
            CHECK (result.getPixelAt (4, 5).getBrightness() == Catch::Approx (0.0));
        }

        // the differences between corners have to do with edge bleed of the stack blur algorithm?
        SECTION ("corners")
        {
            // check top left corner for blur
            CHECK (result.getPixelAt (2, 2).getBrightness() == Catch::Approx (0.890196078).margin (0.01));

            // rest of corners
            CHECK (result.getPixelAt (7, 2).getBrightness() == Catch::Approx (0.964705882).margin (0.01)); // right top
            CHECK (result.getPixelAt (7, 7).getBrightness() == Catch::Approx (0.988235294).margin (0.01)); // right bottom
            CHECK (result.getPixelAt (2, 7).getBrightness() == Catch::Approx (0.964705882).margin (0.01)); // left bottom
        }
    }

    SECTION ("offset")
    {
        g.fillAll (juce::Colours::white);

        SECTION ("positive 1px for x/y")
        {
            // offset by 1px to the right/bottom
            melatonin::DropShadow shadow = { { juce::Colours::black, 2, { 1, 1 } } };
            shadow.render (g, p);
            g.setColour (juce::Colours::black);
            g.fillPath (p);

            save_test_image (result, "positive 1px offset");

            SECTION ("left and top edges have an extra white pixel")
            {
                // left and top edges should have 1 extra white pixel
                CHECK (result.getPixelAt (0, 4).toDisplayString (true) == "FFFFFFFF");
                CHECK (result.getPixelAt (1, 4).toDisplayString (true) == "FFFFFFFF");

                CHECK (result.getPixelAt (4, 0).toDisplayString (true) == "FFFFFFFF");
                CHECK (result.getPixelAt (4, 1).toDisplayString (true) == "FFFFFFFF");
            }

            SECTION ("right and bottom edges lose a white pixel")
            {
                CHECK (result.getPixelAt (8, 4).toDisplayString (true) != "FFFFFFFF");
                CHECK (result.getPixelAt (7, 4).toDisplayString (true) != "FFFFFFFF");

                CHECK (result.getPixelAt (4, 8).toDisplayString (true) != "FFFFFFFF");
                CHECK (result.getPixelAt (4, 7).toDisplayString (true) != "FFFFFFFF");
            }
        }

        SECTION ("negative offset")
        {
            SECTION ("negative Y offset means top pixel is no longer white")
            {
                melatonin::DropShadow shadow = { { juce::Colours::black, 2, { 0, -1 } } };
                shadow.render (g, p);
                g.setColour (juce::Colours::black);
                g.fillPath (p);
                CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (0.91372549).margin (0.01)); // 1st px of blur
            }

            SECTION ("negative X offset means left pixel is no longer white")
            {
                melatonin::DropShadow shadow = { { juce::Colours::black, 2, { -1, 0 } } };
                shadow.render (g, p);
                g.setColour (juce::Colours::black);
                g.fillPath (p);
                CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (0.91372549).margin (.01)); // 1st px of blur
            }
        }
    }

    SECTION ("spread")
    {
        g.fillAll (juce::Colours::white);

        SECTION ("positive")
        {
            melatonin::DropShadow shadow = { { juce::Colours::black, 2, {}, 2 } };
            shadow.render (g, p);
            g.setColour (juce::Colours::black);
            g.fillPath (p);

            SECTION ("no more white pixels, since the blur has spread out")
            {
                for (auto i = 0; i < result.getWidth(); ++i)
                {
                    for (auto j = 0; j < result.getHeight(); ++j)
                    {
                        CHECK (result.getPixelAt (i, j).toDisplayString (true) != "FFFFFFFF");
                    }
                }
            }
        }

        SECTION ("negative")
        {
            SECTION ("reduces the size of the blur by 1px")
            {
                melatonin::DropShadow shadow = { { juce::Colours::black, 2, {}, -1 } };
                shadow.render (g, p);
                g.setColour (juce::Colours::black);
                g.fillPath (p);

                // extra white pixel, as if path were smaller (because it is)
                CHECK (result.getPixelAt (0, 4).toDisplayString (true) == "FFFFFFFF");
                CHECK (result.getPixelAt (1, 4).toDisplayString (true) == "FFFFFFFF");

                // blur starts 1px before the path now
                CHECK (result.getPixelAt (2, 4).toDisplayString (true) != "FFFFFFFF");
            }

            SECTION ("cancels out the blur when -spread = radius")
            {
                // spread can't be more than -1 in our example, since our path is 3x3
                melatonin::DropShadow shadow = { { juce::Colours::black, 1, {}, -1 } };

                shadow.render (g, p);
                g.setColour (juce::Colours::black);
                g.fillPath (p);

                CHECK (result.getPixelAt (2, 2).toDisplayString (true) == "FFFFFFFF");
                CHECK (result.getPixelAt (2, 3).toDisplayString (true) == "FFFFFFFF");
            }
        }
    }

    SECTION ("multiple shadow colors")
    {
        g.fillAll (juce::Colours::white);

        // dammit, in JUCE "lime" is actually pure green...
        melatonin::DropShadow shadow = { { juce::Colours::red, 2 }, { juce::Colours::lime, 2 } };

        SECTION ("to start, our context is white")
        {
            auto color = result.getPixelAt (4, 4);
            CHECK (color.getFloatRed() == Catch::Approx (1.0f));
            CHECK (color.getFloatGreen() == Catch::Approx (1.0f));
            CHECK (color.getFloatBlue() == Catch::Approx (1.0f));
        }

        // TODO: figure out red/green discrepancy
        SECTION ("post shadow, red and green are present", "[.]")
        {
            shadow.render (g, p);
            auto color = result.getPixelAt (4, 4);
            CHECK (color.getFloatRed() == Catch::Approx (0.4f)); // TODO: no idea wtf
            CHECK (color.getFloatGreen() == Catch::Approx (0.76471f).margin (0.005));
            CHECK (color.getFloatBlue() == Catch::Approx (0.16078f).margin (0.005));
        }
    }

    SECTION ("Alpha")
    {
        g.fillAll (juce::Colours::white);

        SECTION ("alpha of 0 is invisible")
        {
            melatonin::DropShadow shadow = { { juce::Colours::black.withAlpha (0.0f), 2 } };
            shadow.render (g, p);

            // the entire image is still white
            CHECK (isImageFilled (result, juce::Colours::white) == true);
        }

        SECTION ("alpha of 1 is opaque")
        {
            melatonin::DropShadow shadow = { { juce::Colours::black.withAlpha (1.0f), 2 } };
            shadow.render (g, p);

            // center pixel of blur is not black (0), but pretty dark still
            CHECK (result.getPixelAt (4, 4).getLightness() == Catch::Approx (0.39608f).margin (0.01));
        }

        SECTION ("alpha of 0.5 is translucent")
        {
            melatonin::DropShadow shadow = { { juce::Colours::black.withAlpha (0.5f), 2 } };
            shadow.render (g, p);

            // center pixel of blur is much lighter (remember, we're on white!)
            CHECK (result.getPixelAt (4, 4).getLightness() == Catch::Approx (0.69804f).margin (0.01));
        }
    }
}

#if JUCE_MAC
// Verify what macOS and JUCE are doing at the raw pixel level
TEST_CASE ("Melatonin Blur JUCE premultiplied check")
{
    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);
    g.fillAll (juce::Colours::white);
    g.drawImageAt (result, 0, 0);

    juce::Image singleChannel (juce::Image::SingleChannel, 9, 9, true);
    juce::Graphics g2 (singleChannel);

    SECTION ("sanity check juce's premultiplied-ness")
    {
        uint8_t alpha = 85u; // 55 in hex
        g.fillAll (juce::Colours::black);

        auto color = juce::Colours::white.withAlpha (alpha);
        g.fillAll (color);

        // our color is white, with an alpha
        SECTION ("colour is white with an alpha of 55")
        {
            CHECK (color.toDisplayString (true) == "55FFFFFF");
        }

        SECTION ("getPixelAt returns an un-premultiplied color")
        {
            CHECK (result.getPixelAt (0, 0).toDisplayString (true) == "FF555555");
        }

        SECTION ("actual pixels are premultiplied")
        {
            juce::Image::BitmapData resultData (result, juce::Image::BitmapData::readWrite);
            auto actualPixel = getActualARGBPixel (resultData.getPixelPointer (0, 0));
            CHECK (actualPixel.a == 255u);
            CHECK (actualPixel.r == 85u);
            CHECK (actualPixel.g == 85u);
            CHECK (actualPixel.b == 85u);
        }
    }

    SECTION ("sanity check vimage's premultiplied function")
    {
        juce::Image dst (juce::Image::ARGB, 1, 1, true);
        juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);
        vImage_Buffer dstBuffer = { dstData.getLinePointer (0), 1, 1, 4 };
        auto firstPixel = dstData.getPixelPointer (0, 0);
        setActualPixel (firstPixel, { 85, 255, 255, 255 });
        auto actualPixel = getActualARGBPixel (firstPixel);

        SECTION ("before premultiply")
        {
            SECTION ("Alpha is untouched")
            {
                CHECK (actualPixel.a == 85u);
            }

            SECTION ("RGB are modified")
            {
                CHECK (actualPixel.r == 255u);
                CHECK (actualPixel.g == 255u);
                CHECK (actualPixel.b == 255u);
            }

            // Lesson learned: don't use getPixelAt to verify tests
            SECTION ("JUCE's getPixelAt presents as a un-premultiplied Colour")
            {
                CHECK (dst.getPixelAt (0, 0).toDisplayString (true) == "55FFFFFF");
            }
        }

        SECTION ("after premultiply")
        {
            vImagePremultiplyData_BGRA8888 (&dstBuffer, &dstBuffer, kvImageNoFlags);
            actualPixel = getActualARGBPixel (firstPixel);
            SECTION ("Alpha is untouched")
            {
                CHECK (actualPixel.a == 85u);
            }

            SECTION ("RGB are modified")
            {
                CHECK (actualPixel.r == 85u);
                CHECK (actualPixel.g == 85u);
                CHECK (actualPixel.b == 85u);
            }

            // Lesson learned: don't use getPixelAt to verify tests
            SECTION ("JUCE's getPixelAt still presents as a un-premultiplied Colour")
            {
                CHECK (dst.getPixelAt (0, 0).toDisplayString (true) == "55FFFFFF");
            }
        }
    }

    SECTION ("red at full opacity")
    {
        g2.fillAll (juce::Colours::white); // populate the single channel with all at 255
        auto converted = melatonin::blur::convertToARGB (singleChannel, juce::Colours::red);
        juce::Image::BitmapData data (converted, juce::Image::BitmapData::readWrite);

        CHECK (converted.getPixelAt (0, 0).toDisplayString (true) == "FFFF0000");
        auto actualPixel = getActualARGBPixel (data.getPixelPointer (0, 0));
        CHECK (actualPixel.a == 255u);
        CHECK (actualPixel.r == 255u);
        CHECK (actualPixel.g == 0u);
        CHECK (actualPixel.b == 0u);
    }

    SECTION ("red at 1/3 opacity")
    {
        uint8_t alpha = 85u; // 55 in hex
        g2.fillAll (juce::Colours::white.withAlpha (alpha));
        auto converted = melatonin::blur::convertToARGB (singleChannel, juce::Colours::red);
        juce::Image::BitmapData data (converted, juce::Image::BitmapData::readWrite);

        auto actualPixel = getActualARGBPixel (data.getPixelPointer (0, 0));
        CHECK (actualPixel.a == 85u);
        CHECK (actualPixel.r == 85u);
        CHECK (actualPixel.g == 0u);
        CHECK (actualPixel.b == 0u);
    }
};
#endif
