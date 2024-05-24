#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur PathWithShadows")
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

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);
    juce::Graphics g (result);
    g.fillAll (juce::Colours::white);

    melatonin::PathWithShadows p;

    // stick the 3x3 box centered inside a 9x9
    p.path.addRectangle (bounds.translated (3, 3));

    SECTION ("renders the path")
    {
        p.render (g);
        CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 3, 3).toString());
    }

    SECTION ("renders a 1px drop shadow")
    {
        p.dropShadow.setColor (juce::Colours::red).setRadius (1);
        p.render (g);
        CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
    }

    SECTION ("path renders in front of drop shadow")
    {
        p.dropShadow.setColor (juce::Colours::red).setRadius (1);
        p.render (g);

        // check for the black square
        CHECK (getPixels (result, 3, { 3, 5 }) == "FF000000, FF000000, FF000000");
        CHECK (getPixels (result, { 3, 5 }, 3) == "FF000000, FF000000, FF000000");
    }

    SECTION ("1px inner shadow")
    {
        p.innerShadow.setColor (juce::Colours::red).setRadius (1);
        p.render (g);

        // still is 3x3
        CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 3, 3).toString());

        // outside edges of square replaced with red
        // using float values + greater than because exact amounts differ on windows
        CHECK (result.getPixelAt (3,3).getFloatRed() > 0.4f);
        CHECK (result.getPixelAt (3,5).getFloatRed() > 0.4f);
        CHECK (result.getPixelAt (5,5).getFloatRed() > 0.4f);
        CHECK (result.getPixelAt (5,3).getFloatRed() > 0.4f);
    }
}
