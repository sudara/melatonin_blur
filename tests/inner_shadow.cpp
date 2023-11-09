#include "../melatonin/implementations/gin.h"
#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Inner Shadow")
{
    // Test Image (differs from drop shadow, has more "meat")

    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  1  1  1  1  1  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0

    // create a 4px by 4px square
    auto bounds = juce::Rectangle<float> (4, 4);
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

    SECTION ("reference implementation inner shadow")
    {
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);
        g.fillPath (p);

        // inner shadow renders AFTER the path render!
        melatonin::stackBlur::renderInnerShadow (g, p, juce::Colours::black, 2);

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

        SECTION ("path edges are blurred (no longer black)")
        {
            CHECK (getPixel (result, 2, 2) != "FF000000");
            CHECK (getPixels (result, 2, { 2, 6 }) == "FF000000, FF000000, FF000000, FF000000, FF000000");
        }

        SECTION ("center value (more than 2px from edge) is white")
        {
            CHECK (getPixel (result, 4, 4) == "FFFFFFFF");
        }
    }

    SECTION ("Melatonin InnerShadow")
    {
        g.fillAll (juce::Colours::white);
        melatonin::InnerShadow shadow = { { juce::Colours::black, 2 } };
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

        SECTION ("path edges are blurred (no longer black)")
        {
            CHECK (getPixel (result, 2, 2) != "FF000000");
            CHECK (getPixels (result, 2, { 2, 6 }) == "FF000000, FF000000, FF000000, FF000000, FF000000");
        }

        SECTION ("center value (more than 2px from edge) is white")
        {
            CHECK (getPixel (result, 4, 4) == "FFFFFFFF");
        }
    }
}
