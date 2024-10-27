#include "../melatonin/internal/implementations.h"
#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

static void render (melatonin::DropShadow& shadow, juce::Image& result, juce::Path& p)
{
    juce::Graphics g (result);
    g.fillAll (juce::Colours::white);
    shadow.render (g, p);
}

TEST_CASE ("Melatonin Blur Setters")
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

    SECTION ("Setters")
    {
        melatonin::DropShadow shadow;

        SECTION ("setOffset")
        {
            shadow.setRadius (1);
            render (shadow, result, p);

            SECTION ("works")
            {
                // check bounds of non-white rectangle
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());

                shadow.setOffset ({ 2, 2 });
                render (shadow, result, p);

                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (4, 4, 5, 5).toString());
            }

            SECTION ("takes int points")
            {
                auto intPoint = juce::Point<int> { 2, 2 };
                shadow.setOffset (intPoint);
            }

            SECTION ("takes int literals in brace init")
            {
                shadow.setOffset ({ 2, 2 });
            }

            SECTION ("takes floating point Point")
            {
                juce::Point floatPoint { 2.f, 2.f };
                shadow.setOffset (floatPoint);
            }

            SECTION ("takes float literals in brace init ")
            {
                shadow.setOffset ({ 2.f, 2.f });
            }

            SECTION ("takes double literals in brace init")
            {
                shadow.setOffset ({ 2.0, 2.0 });
            }

            SECTION ("takes Point<double>")
            {
                shadow.setOffset (juce::Point<double> { 2.0, 2.0 });
            }

            SECTION ("takes x,y integers")
            {
                shadow.setOffset (1, 2);
            }

            SECTION ("takes x,y floats")
            {
                shadow.setOffset (1.f, 2.f);
            }

            SECTION ("takes x,y doubles")
            {
                shadow.setOffset (1.0, 2.0);
            }
        }

        SECTION ("setRadius")
        {
            shadow.setRadius (2);
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
        }

        // this just modifies the color of the shadow
        SECTION ("setOpacity")
        {
            shadow.setRadius (2);
            shadow.setOpacity (0.f);
            render (shadow, result, p);
            // we aren't drawing the path, so nothing is drawn
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (0, 0, 0, 0).toString());
        }

        SECTION ("setSpread")
        {
            shadow.setRadius (1);
            shadow.setSpread (1);
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
        }

        SECTION ("setColor")
        {
            shadow.setRadius (1);
            shadow.setColor (juce::Colours::red);
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
            save_test_image (result, "setters_color.png");
            CHECK (getPixels (result, 4, { 4, 4 }) == "FFFF0000");
        }
    }

    SECTION ("setters and cache")
    {
        melatonin::DropShadow shadow;
        shadow.setRadius (1);
        render (shadow, result, p);

        SECTION ("radius")
        {
            SECTION ("keeping radius same doesn't break cache")
            {
                shadow.setRadius (1); // doesn't break cache
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == false);
            }

            SECTION ("changing radius means recalculation")
            {
                shadow.setRadius (2);
                CHECK (shadow.willRecalculate() == true);
            }
        }

        SECTION ("offset")
        {
            SECTION ("keeping offset same doesn't break cache")
            {
                shadow.setOffset ({ 0, 0 }); // doesn't break cache
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == false);
            }

            SECTION ("changing offset breaks composite cache")
            {
                shadow.setOffset ({ 1, 1 }); // breaks composite cache
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == true);
            }
        }

        SECTION ("color")
        {
            SECTION ("keeping color the same doesn't break cache")
            {
                shadow.setColor (juce::Colours::black);
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == false);
            }

            SECTION ("changing color means recompositing")
            {
                shadow.setColor (juce::Colours::red);
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == true);
            }
        }

        // Regression test, this was broken during 2024
        SECTION ("sequential setters don't confuse cache state")
        {
            SECTION ("setColor with new color then setOpacity with same opacity breaks cache")
            {
                shadow.setColor (juce::Colours::red);
                shadow.setOpacity (1.0f);
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == true);
            }

            SECTION ("setOpacity with new opacity then setColor with same color breaks cache")
            {
                shadow.setOpacity (0.5f);
                shadow.setColor (juce::Colours::black.withAlpha (0.5f));
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == true);
            }

            SECTION ("setRadius with new radius then setSpread with same spread still breaks cache")
            {
                shadow.setRadius (2);
                shadow.setSpread (0);
                CHECK (shadow.willRecalculate() == true);
                CHECK (shadow.willRecomposite() == false);
            }
        }
    }
}
