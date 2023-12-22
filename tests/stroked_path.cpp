#include "../melatonin/implementations/gin.h"
#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// All of these tests operate @1x
TEST_CASE ("Melatonin Blur Stroked Path")
{
    // Test Image is a 5 pixel diagonal stroke
    // 0 is white, 1 is black

    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  1  0  0
    // 0  0  0  0  0  1  0  0  0
    // 0  0  0  0  1  0  0  0  0
    // 0  0  0  1  0  0  0  0  0
    // 0  0  1  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0

    juce::Path p;

    // stick the 5x5 line centered inside a 9x9
    p.addLineSegment (juce::Line<float> (2, 6, 6, 2), 1.0f);

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);

    // TODO: sorta surprised this doesn't stroke the center by default, JUCE?
    SECTION ("no shadow")
    {
        g.fillAll (juce::Colours::white);
        auto strokeType = juce::PathStrokeType (2.0f);
        g.strokePath (p, strokeType);

        // the top left corner is all white
        CHECK (getPixels (result, { 0, 1 }, { 0, 1 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

        // the bottom right corner is all white
        CHECK (getPixels (result, { 7, 8 }, { 7, 8 }) == "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");
    }

    // this doesn't test shadow details, just that the API works :)
    SECTION ("drop shadow with stroke of 2")
    {
        g.fillAll (juce::Colours::white);
        melatonin::DropShadow shadow (juce::Colours::black, 2);
        auto strokeType = juce::PathStrokeType (3.0f);
        shadow.renderStroked (g, p, strokeType);
        g.strokePath (p, strokeType);

        save_test_image (result, "stroked_path.png");

        // the top left corner is no longer white
        CHECK (getPixels (result, { 0, 1 }, { 0, 1 }) != "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");

        // the bottom right corner is no longer white
        CHECK (getPixels (result, { 7, 8 }, { 7, 8 }) != "FFFFFFFF, FFFFFFFF, FFFFFFFF, FFFFFFFF");
    }

    SECTION ("changing stroke type breaks cache")
    {
        g.fillAll (juce::Colours::white);
        melatonin::DropShadow shadow (juce::Colours::black, 2);
        auto strokeType = juce::PathStrokeType (3.0f);
        shadow.renderStroked (g, p, strokeType);

        // erase the image, render the shadow again
        g.fillAll (juce::Colours::white);
        strokeType = juce::PathStrokeType (0.0f);
        shadow.renderStroked (g, p, strokeType);

        // there should be no more shadow (and we didn't render the path, so pure white)
        CHECK (filledBounds (result).isEmpty());

        // erase the image, render the shadow again
        g.fillAll (juce::Colours::white);
        strokeType = juce::PathStrokeType (1.0f);
        shadow.renderStroked (g, p, strokeType);

        CHECK (!filledBounds (result).isEmpty());
    }

    SECTION ("inner shadow")
    {
        g.fillAll (juce::Colours::white);
        melatonin::InnerShadow shadow (juce::Colours::white, 1);
        auto strokeType = juce::PathStrokeType (3.0f);
        g.strokePath (p, strokeType);
        CHECK (getPixel (result, 2, 6) == "FF000000");
        CHECK (getPixel (result, 3, 5) == "FF000000");
        CHECK (getPixel (result, 4, 4) == "FF000000");
        shadow.renderStroked (g, p, strokeType);
        CHECK (getPixel (result, 2, 6) != "FF000000");
        CHECK (getPixel (result, 3, 5) != "FF000000");
        CHECK (getPixel (result, 4, 4) != "FF000000");
        save_test_image (result, "stroked_path_inner.png");
    }
}
