#include "../melatonin/implementations/gin.h"
#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Inner Shadow")
{
    // Test Image (differs from drop shadow, has more inner "meat")
    // 0 is white, 1 is black, the shadow will be *white* in the center

    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0

    // create a 5px by 5px square
    auto bounds = juce::Rectangle<float> (5, 5);
    juce::Path p;

    // stick the 3x3 box centered inside a 9x9
    p.addRectangle (bounds.translated (2, 2));

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);

    SECTION ("no shadow (sanity check)")
    {
        g.fillAll (juce::Colours::white);

        g.setColour (juce::Colours::black);
        g.fillPath (p);

        // nothing should be outside the top left corner
        REQUIRE (getPixel (result, 1, 1) == "FFFFFFFF"); // ARGB

        // starting at pixel 2,2 we have black
        REQUIRE (getPixel (result, 2, 2) == "FF000000");
    }

    SECTION ("old implementation inner shadow")
    {
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);
        g.fillPath (p);

        // inner shadow renders AFTER the path render!
        melatonin::stackBlur::renderInnerShadow (g, p, juce::Colours::white, 2);

        SECTION ("outside of the path isn't touched (still white)")
        {
            REQUIRE (getPixel (result, 1, 1) == "FFFFFFFF");

            // left edge
            CHECK (getPixels (result, 1, { 2, 6 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // top edge
            CHECK (getPixels (result, { 2, 6 }, 1) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // right edge
            CHECK (getPixels (result, 7, { 2, 6 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // bottom edge
            CHECK (getPixels (result, { 2, 6 }, 7) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");
        }

        SECTION ("path corners are blurred (no longer black)")
        {
            CHECK (getPixel (result, 2, 2) != "FF000000");
            CHECK (getPixel (result, 2, 7) != "FF000000");
            CHECK (getPixel (result, 7, 7) != "FF000000");
            CHECK (getPixel (result, 2, 2) != "FF000000");
        }

        SECTION ("center value (more than 2px from edge) is still black")
        {
            CHECK (getPixel (result, 4, 4) == "FF000000");
        }
    }

    SECTION ("Melatonin InnerShadow 2px")
    {
        g.fillAll (juce::Colours::white);
        melatonin::InnerShadow shadow = { { juce::Colours::white, 2 } };
        g.setColour (juce::Colours::black);
        g.fillPath (p);

        // inner shadow render must come AFTER the path render
        shadow.render (g, p);
        SECTION ("outside of the path isn't touched (still white)")
        {
            REQUIRE (getPixel (result, 1, 1) == "FFFFFFFF");

            // left edge
            CHECK (getPixels (result, 1, { 2, 6 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // top edge
            CHECK (getPixels (result, { 2, 6 }, 1) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // right edge
            CHECK (getPixels (result, 7, { 2, 6 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

            // bottom edge
            CHECK (getPixels (result, { 2, 6 }, 7) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");
        }

        SECTION ("path corners are blurred (no longer black)")
        {
            CHECK (getPixel (result, 2, 2) != "FF000000");
            CHECK (getPixel (result, 2, 7) != "FF000000");
            CHECK (getPixel (result, 7, 7) != "FF000000");
            CHECK (getPixel (result, 2, 2) != "FF000000");
        }

        SECTION ("path corners are symmetrical")
        {
            CHECK (getPixel (result, 2, 2) == getPixel (result, 6, 6));
            CHECK (getPixel (result, 2, 2) == getPixel (result, 6, 2));
            CHECK (getPixel (result, 2, 2) == getPixel (result, 2, 6));
        }

        SECTION ("center value (more than 2px from edge) is still black")
        {
            CHECK (getPixel (result, 4, 4) == "FF000000");
        }

        SECTION ("With spread")
        {
            SECTION ("positive spread adds")
            {
                auto cornerValueWithoutSpread = result.getPixelAt (2, 2).getBrightness();

                // redo the blur with spread
                g.fillAll (juce::Colours::white);
                melatonin::InnerShadow shadowWithPositiveSpread = { { juce::Colours::white, 2, {}, 1 } };
                g.setColour (juce::Colours::black);
                g.fillPath (p);

                // inner shadow render must come AFTER the path render
                shadowWithPositiveSpread.render (g, p);

                SECTION ("increases shadow amount (brightness)")
                {
                    CHECK (result.getPixelAt (2, 2).getBrightness() > cornerValueWithoutSpread);
                }

                SECTION ("center pixel is no longer black")
                {
                    CHECK (getPixel (result, 4, 4) != "FF000000");
                }
            }
        }
    }
}
