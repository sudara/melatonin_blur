#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Composite ARGB")
{
    // needed for JUCE not to pee its pants (aka leak) when working with graphics
    juce::ScopedJuceInitialiser_GUI juce;

    // this tests which parts of the end graphics context actually get painted to
    // which is a proxy for what the argb composite is up to
    SECTION ("filled bounds")
    {
        // boot up the bare minimum of a CachedShadow
        juce::Image context (juce::Image::PixelFormat::ARGB, 20, 20, true);

        SECTION ("match a single channel blur")
        {
            auto s1 = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

            juce::Path p;
            // make a 4x4 at 2,2
            p.addRectangle (juce::Rectangle<float> (2, 2, 4, 4));
            auto shadow = melatonin::DropShadow ({ s1 });

            {
                juce::Graphics g (context);
                g.fillAll (juce::Colours::white);
                shadow.render (g, p);
            }

            // check bounds of non-white rectangle
            CHECK (filledBounds (context) == juce::Rectangle<int> (0, 0, 8, 8));
        }

        SECTION ("applies offset correctly")
        {
            auto s1 = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 2, 2 }, 0 });

            juce::Path p;
            // make a 4x4 at 2,2
            p.addRectangle (juce::Rectangle<float> (2, 2, 4, 4));
            auto shadow = melatonin::DropShadow ({ s1 });

            {
                juce::Graphics g (context);
                g.fillAll (juce::Colours::white);
                shadow.render (g, p);
            }

            save_test_image (context, "offset");

            // check bounds of non-white rectangle
            CHECK (filledBounds (context).toString() == juce::Rectangle<int> (2, 2, 8, 8).toString());
        }

        SECTION ("takes the larger of 2 blur bounds")
        {
            auto s1 = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

            // this has a 2px positive offset on the x axis
            auto s2 = melatonin::ShadowParameters ({ juce::Colours::black, 3, { 0, 0 }, 0 });

            juce::Path p;
            // make a 4x4 at 2,2
            p.addRectangle (juce::Rectangle<float> (3, 3, 4, 4));

            {
                juce::Graphics g (context);
                g.fillAll (juce::Colours::white);
                auto shadow = melatonin::DropShadow ({ s1, s2 });
                shadow.render (g, p);
            }

            // check bounds of non-white rectangle
            CHECK (filledBounds (context).toString() == juce::Rectangle<int> (0, 0, 10, 10).toString());
        }

        SECTION ("takes offset into account when unioning bounds")
        {
            auto s1 = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

            // this has a 2px positive offset on the x axis
            auto s2 = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 2, 0 }, 0 });

            juce::Path p;
            // make a 4x4 at 2,2
            p.addRectangle (juce::Rectangle<float> (2, 2, 4, 4));
            {
                juce::Graphics g (context);
                g.fillAll (juce::Colours::white);

                auto shadow = melatonin::DropShadow ({ s1, s2 });
                shadow.render (g, p);
            }

            // check bounds of non-white rectangle
            // it's just 2px wider
            CHECK (filledBounds (context).toString() == juce::Rectangle<int> (0, 0, 10, 8).toString());
        }

        SECTION ("doesn't freak out with 0 radius")
        {
            auto s1 = melatonin::ShadowParameters ({ juce::Colours::black, 0, { 0, 0 }, 0 });

            juce::Path p;
            // make a 4x4 at 2,2
            p.addRectangle (juce::Rectangle<float> (2, 2, 4, 4));
            auto shadow = melatonin::DropShadow ({ s1 });

            {
                juce::Graphics g (context);
                g.fillAll (juce::Colours::white);

                shadow.render (g, p);
            }

            save_test_image (context, "zero_radius");

            // check bounds of non-white rectangle
            CHECK (filledBounds (context).toString() == juce::Rectangle<int> (2, 2, 4, 4).toString());
        }
    }

    SECTION ("path position is agnostic")
    {
        juce::Image context (juce::Image::PixelFormat::ARGB, 150, 150, true);
        juce::Graphics g (context);
        g.addTransform (juce::AffineTransform::scale (2));
        g.fillAll (juce::Colours::white);

        auto dummyShadow = melatonin::ShadowParameters ({ juce::Colours::black, 2, { 0, 0 }, 0 });

        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (0, 0, 4, 4));

        juce::Path pTranslated;
        pTranslated.addRectangle (juce::Rectangle<float> (100, 100, 4, 4));

        auto shadow = melatonin::InnerShadow (dummyShadow);
        shadow.render (g, p);
        auto originalPath = shadow.lastOriginAgnosticPath;
        shadow.render (g, p);
        auto translatedPath = shadow.lastOriginAgnosticPath;

        CHECK (originalPath == translatedPath);
    }
}
