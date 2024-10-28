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

TEST_CASE ("Melatonin Blur Constructors and Float Parameters")
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

    SECTION ("constructors")
    {
        SECTION ("takes just a radius via direct init")
        {
            melatonin::DropShadow shadow { 2 };
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
        }

        SECTION ("takes multiple radii to setup multiple black shadows")
        {
            melatonin::DropShadow shadows { 1, 2 };
            render (shadows, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
        }

        SECTION ("takes just raw color and radius via direct init")
        {
            melatonin::DropShadow shadow { juce::Colours::black, 1 };
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
        }

        SECTION ("takes just raw color and radius via copy init")
        {
            melatonin::DropShadow shadow = { juce::Colours::black, 1 };
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
        }

        SECTION ("takes raw color and radius and offset")
        {
            melatonin::DropShadow shadow = { juce::Colours::black, 1, { 1, 1 } };
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 5, 5).toString());
        }

        SECTION ("takes a ShadowParameters object via direct and copy init")
        {
            melatonin::DropShadow shadow { { juce::Colours::black, 1, { 1, 1 }, 0 } };
            render (shadow, result, p);
            shadow = { { juce::Colours::black, 1, { 1, 1 }, 0 } };
            render (shadow, result, p);
            CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 5, 5).toString());
        }

        SECTION ("takes multiple ShadowParameters objects via direct and copy init")
        {
            melatonin::DropShadow shadow { { juce::Colours::black, 1, { 1, 1 }, 0 }, { juce::Colours::black, 1, { 1, 1 }, 0 } };
            shadow = { { juce::Colours::black, 1, { 1, 1 }, 0 }, { juce::Colours::black, 1, { 1, 1 }, 0 } };
            render (shadow, result, p);
        }
    }

    SECTION ("float")
    {
        SECTION ("constructors")
        {
            SECTION ("takes just a radius via direct init")
            {
                melatonin::DropShadow shadow { 2.2 };
                shadow = { 2.2f };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
            }

            SECTION ("takes multiple radii to setup multiple black shadows")
            {
                melatonin::DropShadow shadows { 1.2f, 2.5f };
                shadows = { 1.2, 2.5 };
                render (shadows, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
            }

            SECTION ("takes a ShadowParameters object via direct and copy init")
            {
                melatonin::DropShadow shadow { { juce::Colours::black, 1.2f, { 1.1f, 1.1f }, 0.1f } };
                render (shadow, result, p);
                shadow = { { juce::Colours::black, 1, { 1, 1 }, 0 } };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 5, 5).toString());
            }

            SECTION ("takes multiple ShadowParameters objects via direct and copy init")
            {
                melatonin::DropShadow shadow { { juce::Colours::black, 1.2f, { 1.1f, 1.1f }, 0.f }, { juce::Colours::black, 1.f, { 1.f, 1.f }, 0.f } };
                shadow = { { juce::Colours::black, 1.f, { 1.f, 1.f }, 0.f }, { juce::Colours::black, 1.f, { 1.f, 1.f }, 0.f } };
                render (shadow, result, p);
            }
        }

        SECTION ("float radius")
        {
            SECTION ("rounds down")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.4f };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
            }

            SECTION ("rounds up")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.6f };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
            }
        }

        SECTION ("float spread")
        {
            SECTION ("rounds down")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.0, { 0, 0 }, 0.4f };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (2, 2, 5, 5).toString());
            }

            SECTION ("rounds up")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.0, { 0, 0 }, 0.6f };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (1, 1, 7, 7).toString());
            }
        }

        SECTION ("float offset")
        {
            SECTION ("rounds down")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.0f, { 1.4f, 1.4f } };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (3, 3, 5, 5).toString());
            }

            SECTION ("rounds up")
            {
                melatonin::DropShadow shadow = { juce::Colours::black, 1.0f, { 1.6f, 1.6f } };
                render (shadow, result, p);
                CHECK (filledBounds (result).toString() == juce::Rectangle<int> (4, 4, 5, 5).toString());
            }
        }
    }

    SECTION ("float setters")
    {
        melatonin::DropShadow shadow;
        juce::Image floatResult (juce::Image::ARGB, 9, 9, true);

        SECTION ("radius")
        {
            shadow.setRadius (1);
            render (shadow, result, p);

            shadow.setRadius (1.4f);
            CHECK (shadow.willRecalculate() == false);
            render (shadow, floatResult, p);

            CHECK (imagesAreIdentical (result, floatResult));
        }

        SECTION ("spread")
        {
            shadow.setSpread (1);
            render (shadow, result, p);

            shadow.setSpread (1.4f);
            CHECK (shadow.willRecalculate() == false);
            render (shadow, floatResult, p);

            CHECK (imagesAreIdentical (result, floatResult));
        }

        SECTION ("offset")
        {
            shadow.setOffset ({ 1, 1 });
            render (shadow, result, p);

            shadow.setOffset ( { 1.4f, 1.4f });
            CHECK (shadow.willRecomposite() == false);
            render (shadow, floatResult, p);

            CHECK (imagesAreIdentical (result, floatResult));
        }
    }
}
