#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// most of the shadow implementation is tested as a part of Drop/Inner shadow tests
// but we're testing juce::Image sizing / scaling here
TEST_CASE ("Melatonin Blur renderShadowToARGB")
{
    auto dummyShadow = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

    SECTION ("no scaling")
    {
        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (4, 4));
        auto result = melatonin::renderShadowToARGB (dummyShadow, p, 1);

        CHECK (result.isARGB() == true);

        // path size + 2*shadow radius
        CHECK (result.getWidth() == 8);
        CHECK (result.getHeight() == 8);
    }

    SECTION ("integer scaling")
    {
        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (4, 4));
        auto result = melatonin::renderShadowToARGB (dummyShadow, p, 2);

        CHECK (result.isARGB() == true);

        // (path size + 2*shadow radius) * scale
        CHECK (result.getWidth() == 16);
        CHECK (result.getHeight() == 16);
    }

    SECTION ("wacky windows 1.5 scaling")
    {
        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (4, 4));
        auto result = melatonin::renderShadowToARGB (dummyShadow, p, 1.5);

        CHECK (result.isARGB() == true);

        // (path size + 2*shadow radius) * scale
        CHECK (result.getWidth() == 12);
        CHECK (result.getHeight() == 12);
    }
}
