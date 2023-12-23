#include "../melatonin/implementations/gin.h"
#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// All of these tests operate @1x
TEST_CASE ("Melatonin Blur Text Shadow")
{
    // Test Image is a 9x9 with 0 in the center
    // 0 is white, 1 is black

    // Grid is 9x9, and we're drawing a big centered O

    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0
    // 0  0  0  0  0  0  0  0  0

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);

    SECTION ("no shadow")
    {
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);
        g.drawText ("O", 0, 0, 9, 9, juce::Justification::centred, false);

        // middle of image is white
        CHECK (result.getPixelAt (4, 4).toDisplayString (true) == "FFFFFFFF");

        // top middle is close to black
        auto topMiddle = result.getPixelAt (4, 4);
        CHECK (getPixel (result, 4, 0) != "FFFFFFFF");
        CHECK (topMiddle.getRed() == topMiddle.getGreen());
    }

    // not testing mechanics/details of drop shadow here, just that API worked
    SECTION ("drop shadow")
    {
        g.fillAll (juce::Colours::white);
        melatonin::DropShadow shadow (juce::Colours::black, 3);
        g.setColour (juce::Colours::black);
        shadow.render (g, "O", 0, 0, 9, 9, juce::Justification::centred);
        g.drawText ("O", 0, 0, 9, 9, juce::Justification::centred, false);

        // middle of image is no longer white
        CHECK (result.getPixelAt (4, 4).toDisplayString (true) != "FFFFFFFF");
        save_test_image (result, "text_shadow.png");
    }

    SECTION ("inner shadow")
    {
        g.fillAll (juce::Colours::white);
        melatonin::InnerShadow shadow (juce::Colours::red, 1);
        g.setColour (juce::Colours::black);
        g.drawText ("O", 0, 0, 9, 9, juce::Justification::centred, false);
        shadow.render (g, "O", 0, 0, 9, 9, juce::Justification::centred);

        // top middle has more red than green
        auto topMiddle = result.getPixelAt (4, 0);
        CHECK (topMiddle.getRed() > topMiddle.getGreen());
        save_test_image (result, "text_inner_shadow.png");
    }

    SECTION ("accepts permutations of rectangle")
    {
        melatonin::InnerShadow shadow (juce::Colours::red, 1);
        shadow.render (g, "O", 0, 0, 9, 9, juce::Justification::centred);
        juce::Rectangle<int> intRect (0, 0, 9, 9);
        shadow.render (g, "O", intRect, juce::Justification::centred);
        juce::Rectangle<float> floatRect (0, 0, 9, 9);
        shadow.render (g, "O", floatRect, juce::Justification::centred);
    }
}
