#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// most of the shadow implementation is tested as a part of Drop/Inner shadow tests
// but we're testing juce::Image sizing / scaling here
TEST_CASE ("Melatonin Blur Render To Single Channel")
{
    using namespace melatonin::internal;
    auto dummyShadow = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

    juce::Path p;
    p.addRectangle (juce::Rectangle<float> (4, 4));

    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    SECTION ("no scaling")
    {
        auto result = RenderedSingleChannelShadow (dummyShadow).render (p, 1);

        CHECK (result.isSingleChannel() == true);

        // path size + 2*shadow radius
        CHECK (result.getWidth() == 8);
        CHECK (result.getHeight() == 8);
    }

    SECTION ("integer scaling")
    {
        auto result = RenderedSingleChannelShadow (dummyShadow).render (p, 2);

        CHECK (result.isSingleChannel() == true);

        // (path size + 2*shadow radius) * scale
        CHECK (result.getWidth() == 16);
        CHECK (result.getHeight() == 16);
    }

    SECTION ("wacky windows 1.5 scaling")
    {
        auto result = RenderedSingleChannelShadow (dummyShadow).render (p, 1.5);

        CHECK (result.isSingleChannel() == true);

        // rounded ((path size + 2*shadow radius) * scale) + 2 extra pixels with subpixel bleed
        CHECK (result.getWidth() == 14);
        CHECK (result.getHeight() == 14);
    }

    SECTION ("alpha is irrelevant to single channel render")
    {
        auto result = RenderedSingleChannelShadow (dummyShadow).render (p, 1);

        auto lowerAlpha = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });
        auto resultWithLowerAlpha = RenderedSingleChannelShadow (lowerAlpha).render (p, 1);

        // check each pixel
        for (auto x = 0; x < result.getWidth(); ++x)
        {
            for (auto y = 0; y < result.getHeight(); ++y)
            {
                auto expectedPixel = result.getPixelAt (x, y).toString();
                auto actualPixel = resultWithLowerAlpha.getPixelAt (x, y).toString();

                CHECK (actualPixel == expectedPixel);
            }
        }
    }

    SECTION ("offset is irrelevant to single channel render")
    {
        auto result = RenderedSingleChannelShadow (dummyShadow).render (p, 1);

        auto offsetShadow = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 2, 2 }, 0 });
        auto resultWithOffset = RenderedSingleChannelShadow (offsetShadow).render (p, 1);

        // TODO: this isn't working for some reason, even when bounds differ, these pass
        CHECK (result.getWidth() == resultWithOffset.getWidth());
        CHECK (result.getHeight() == resultWithOffset.getHeight());

        // check each pixel
        for (auto x = 0; x < result.getWidth(); ++x)
        {
            for (auto y = 0; y < result.getHeight(); ++y)
            {
                auto expectedPixel = result.getPixelAt (x, y).toString();
                auto actualPixel = resultWithOffset.getPixelAt (x, y).toString();

                CHECK (actualPixel == expectedPixel);
            }
        }
    }

    SECTION ("scaledShadowBounds")
    {
        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (4, 4));

        SECTION ("is set after render")
        {
            auto shadow = RenderedSingleChannelShadow (dummyShadow);
            CHECK (shadow.getScaledBounds().isEmpty() == true);
            shadow.render (p, 1);
            CHECK (shadow.getScaledBounds().isEmpty() == false);
        }

        SECTION ("scales with incoming scale")
        {
            auto shadow = RenderedSingleChannelShadow (dummyShadow);
            shadow.render (p, 1);
            CHECK (shadow.getScaledBounds().getWidth() == 8);
            shadow.render (p, 2);
            CHECK (shadow.getScaledBounds().getWidth() == 16);
        }

        SECTION ("accounts for scaled shadow offset")
        {
            auto shadow = RenderedSingleChannelShadow ({ juce::Colours::black, 2, { 3, 3 }, 0 });
            shadow.render (p, 1);
            CHECK (shadow.getScaledBounds().getX() == 1);
            CHECK (shadow.getScaledBounds().getY() == 1);
            shadow.render (p, 2);
            CHECK (shadow.getScaledBounds().getX() == 2);
            CHECK (shadow.getScaledBounds().getY() == 2);
        }
    }
}
