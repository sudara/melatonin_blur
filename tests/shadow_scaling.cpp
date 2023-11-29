#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Shadow Scaling")
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

    SECTION ("scaling")
    {
        SECTION ("default scaling (tests not running on a screen) is 1.0")
        {
            juce::Image result (juce::Image::ARGB, 9, 9, true);
            juce::Graphics g (result);
            g.fillAll (juce::Colours::white);

            // offset by 1px to the right/bottom
            melatonin::DropShadow shadow = { { juce::Colours::black, 1 } };
            shadow.render (g, p);
            g.setColour (juce::Colours::black);
            g.fillPath (p);

            // 2 lines of white pixels on all edges
            SECTION ("edges are white")
            {
                // left
                CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0f));
                CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (1.0f));

                // top
                CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
                CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (1.0f));

                // right
                CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
                CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (1.0f));

                // bottom
                CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
                CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (1.0f));
            }

            SECTION ("3 pixels into center, edges are blurred")
            {
                CHECK (result.getPixelAt (2, 4).getBrightness() == Catch::Approx (0.74902)); // completely white
                CHECK (result.getPixelAt (4, 2).getBrightness() == Catch::Approx (0.74902).margin (0.01)); // 1st px blur
                CHECK (result.getPixelAt (6, 4).getBrightness() == Catch::Approx (0.74902).margin (0.01)); // 2nd px blur
                CHECK (result.getPixelAt (4, 6).getBrightness() == Catch::Approx (0.74902));
            }
        }

        SECTION ("system scaling impacts resolution")
        {
            float scale = 4.0;

            auto contextWidth = juce::roundToInt (9 * scale);
            auto contextHeight = juce::roundToInt (9 * scale);
            juce::Image result (juce::Image::ARGB, contextWidth, contextHeight, true);
            juce::Graphics g (result);

            // fake a 4x resolution on the main context
            g.addTransform (juce::AffineTransform::scale (scale));
            g.fillAll (juce::Colours::white);

            // internally ARGB images are used for compositing
            SECTION ("@4x")
            {
                SECTION ("default (high fidelity)")
                {
                    // this should be a 8px radius blur at the image level
                    melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };

                    // all path coordinates upscaled, the origin 3,3 will be 12,12 for @4x
                    shadow.render (g, p);
                    g.setColour (juce::Colours::black);
                    g.fillPath (p);

                    save_test_image (result, "happy4x");

                    // there should be no white edges left! (the blur is 4px radius)

                    // left
                    CHECK (result.getPixelAt (0, juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (1, juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));

                    // top
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), 0).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), 1).getBrightness() == Catch::Approx (1.0f));

                    // right
                    CHECK (result.getPixelAt (juce::roundToInt (8 * scale), juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (7 * scale), juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));

                    // bottom
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), juce::roundToInt (8 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), juce::roundToInt (7 * scale)).getBrightness() == Catch::Approx (1.0f));
                }

                SECTION ("lowQuality")
                {
                    melatonin::DropShadow shadow = { { juce::Colours::black, 2, {}, 0, true } };

                    shadow.render (g, p);
                    g.setColour (juce::Colours::black);
                    g.fillPath (p);

                    save_test_image (result, "4x lofi");

                    // there should be no white edges left! (the blur is 4px radius)

                    // left
                    CHECK (result.getPixelAt (0, juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (1, juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));

                    // top
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), 0).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), 1).getBrightness() == Catch::Approx (1.0f));

                    // right
                    CHECK (result.getPixelAt (juce::roundToInt (8 * scale), juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (7 * scale), juce::roundToInt (4 * scale)).getBrightness() == Catch::Approx (1.0f));

                    // bottom
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), juce::roundToInt (8 * scale)).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (juce::roundToInt (4 * scale), juce::roundToInt (7 * scale)).getBrightness() == Catch::Approx (1.0f));
                }
            }

            SECTION ("retina @2x")
            {
                SECTION ("double the radius of 1 is 2")
                {
                    // this now behaves as if the radius is 2
                    melatonin::DropShadow shadow = { { juce::Colours::black, 1 } };
                    shadow.render (g, p);
                    g.setColour (juce::Colours::black);
                    g.fillPath (p);

                    // left
                    CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91373f));

                    // top
                    CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91373f));

                    // right
                    CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91373f));

                    // bottom
                    CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91373f));
                }
            }

            SECTION ("wacky windows @1.5x")
            {
                SECTION ("1.5x the radius of 2 has a radius of 3")
                {
                    // this now behaves as if the radius is 3
                    melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };
                    shadow.render (g, p);
                    g.setColour (juce::Colours::black);
                    g.fillPath (p);

                    // there should be no more white pixels left

                    // left
                    CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (0.96078f));
                    CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.88235f));

                    // top
                    CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (0.96078f));
                    CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.88235f));

                    // right
                    CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (0.96078f));
                    CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.88235f));

                    // bottom
                    CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (0.96078f));
                    CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.88235f));
                }

                SECTION ("1.5x the radius of 1 rounds up to a radius of 2")
                {
                    // this now behaves as if the radius is 2
                    melatonin::DropShadow shadow = { { juce::Colours::black, 1 } };
                    shadow.render (g, p);
                    g.setColour (juce::Colours::black);
                    g.fillPath (p);

                    // left
                    CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91373f));

                    // top
                    CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91373f));

                    // right
                    CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91373f));

                    // bottom
                    CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
                    CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91373f));
                }
            }
        }
    }

    // User scaling = user has wrapped the editor in an AffineTransform
    // Aka, it's lossy upscaling, it doesn't actually use a larger context
    // JUCE recommends this route for app scaling (upscaling in particular is questionable though)
    SECTION ("user scaling isn't relevant")
    {
        auto contextWidth = juce::roundToInt (9);
        auto contextHeight = juce::roundToInt (9);
        juce::Image result (juce::Image::ARGB, contextWidth, contextHeight, true);
        juce::Graphics g (result);

        g.fillAll (juce::Colours::white);

        SECTION ("user scales up")
        {
            juce::Desktop::getInstance().setGlobalScaleFactor (2.0);

            // this still behaves as if the radius is 2
            melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };
            shadow.render (g, p);
            g.setColour (juce::Colours::black);
            g.fillPath (p);

            // left
            CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91373f));

            // top
            CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91373f));

            // right
            CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91373f));

            // bottom
            CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91373f));
        }

        SECTION ("user scales down")
        {
            juce::Desktop::getInstance().setGlobalScaleFactor (0.5);

            // this now behaves as if the radius is 2
            melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };
            shadow.render (g, p);
            g.setColour (juce::Colours::black);
            g.fillPath (p);

            // left
            CHECK (result.getPixelAt (0, 4).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91373f));

            // top
            CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91373f));

            // right
            CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91373f));

            // bottom
            CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91373f));
        }
    }
}
