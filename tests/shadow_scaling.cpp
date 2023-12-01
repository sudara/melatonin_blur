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

            save_test_image(result, "at1x");

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
            std::vector<float> scales { 1, 2, 4 };

            for (auto scale : scales)
            {
                DYNAMIC_SECTION ("system resolution @ " << scale)
                {
                    // specify resolution for the main graphics context
                    // simulates running on a retina or scaled screen
                    auto contextWidth = juce::roundToInt (9 * scale);
                    auto contextHeight = juce::roundToInt (9 * scale);
                    juce::Image result (juce::Image::ARGB, contextWidth, contextHeight, true);
                    juce::Graphics g (result);
                    g.addTransform (juce::AffineTransform::scale (scale));
                    g.fillAll (juce::Colours::white);

                    SECTION ("radius 1")
                    {
                        SECTION ("high fidelity (default)")
                        {
                            // at @4x there will be a 4px radius blur at the image level
                            melatonin::DropShadow shadow = { { juce::Colours::black, 1 } };

                            // all path coordinates upscaled on draw
                            // example: the origin 3,3 will be 12,12 for @4x
                            shadow.render (g, p);
                            g.setColour (juce::Colours::black);
                            g.fillPath (p);

                            save_test_image(result, "at2xbaybee");

                            SECTION ("2 outer edges white")
                            {
                                // left
                                CHECK (getScaledBrightness (result, 0, 4, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 1, 4, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 2, 4, scale) != Catch::Approx (1.0f));

                                // top
                                CHECK (getScaledBrightness (result, 4, 0, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 4, 1, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 4, 2, scale) != Catch::Approx (1.0f));

                                // right
                                CHECK (getScaledBrightness (result, 8, 4, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 7, 4, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 6, 4, scale) != Catch::Approx (1.0f));

                                // bottom
                                CHECK (getScaledBrightness (result, 4, 8, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 4, 7, scale) == Catch::Approx (1.0f));
                                CHECK (getScaledBrightness (result, 4, 6, scale) != Catch::Approx (1.0f));
                            }
                        }

                        SECTION ("lowQuality")
                        {
                            melatonin::DropShadow shadow = { { juce::Colours::black, 1, {}, 0 } };

                            shadow.render (g, p, true);
                            g.setColour (juce::Colours::black);
                            g.fillPath (p);

                            save_test_image(result, "lowQualityRadius1");

                            // left
                            CHECK (getScaledBrightness (result, 0, 4, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 1, 4, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 2, 4, scale) != Catch::Approx (1.0f));

                            // top
                            CHECK (getScaledBrightness (result, 4, 0, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 4, 1, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 4, 2, scale) != Catch::Approx (1.0f));

                            // right
                            CHECK (getScaledBrightness (result, 8, 4, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 7, 4, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 6, 4, scale) != Catch::Approx (1.0f));

                            // bottom
                            CHECK (getScaledBrightness (result, 4, 8, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 4, 7, scale) == Catch::Approx (1.0f));
                            CHECK (getScaledBrightness (result, 4, 6, scale) != Catch::Approx (1.0f));
                        }
                    }
                }
            }
        }

        SECTION ("wacky windows @1.5x")
        {
            auto scale = 1.5f;
            auto contextWidth = juce::roundToInt (9 * scale);
            auto contextHeight = juce::roundToInt (9 * scale);
            juce::Image result (juce::Image::ARGB, contextWidth, contextHeight, true);
            juce::Graphics g (result);
            g.addTransform (juce::AffineTransform::scale (scale));
            g.fillAll (juce::Colours::white);

            // technically shadows will be a bit stronger at x.5 scale
            SECTION ("1.5x the radius of 1 rounds UP to a radius of 2")
            {
                melatonin::DropShadow shadow = { { juce::Colours::black, 1 } };
                shadow.render (g, p);
                g.setColour (juce::Colours::black);
                g.fillPath (p);

                // left
                CHECK (getScaledBrightness (result, 0, 4, scale) == Catch::Approx (1.0f));
                CHECK (getScaledBrightness (result, 1, 4, scale) != Catch::Approx (1.0f));

                // top
                CHECK (getScaledBrightness (result, 4, 0, scale) == Catch::Approx (1.0f));
                CHECK (getScaledBrightness (result, 4, 1, scale) != Catch::Approx (1.0f));

                // right
                CHECK (getScaledBrightness (result, 8, 4, scale) == Catch::Approx (1.0f));
                CHECK (getScaledBrightness (result, 7, 4, scale) != Catch::Approx (1.0f));

                // bottom
                CHECK (getScaledBrightness (result, 4, 8, scale) == Catch::Approx (1.0f));
                CHECK (getScaledBrightness (result, 4, 7, scale) != Catch::Approx (1.0f));
            }

            SECTION ("1.5x the radius of 2 has a radius of 3")
            {
                // this now behaves as if the radius is 3
                melatonin::DropShadow shadow = { { juce::Colours::black, 2 } };
                shadow.render (g, p);
                g.setColour (juce::Colours::black);

                // this will now have an origin of 4.5, 4.5 and a width of 4.5
                g.fillPath (p);

                save_test_image(result, "shadow1.5");

                // there should be no more white pixels left as the radius is now 3

                // left
                CHECK (getScaledBrightness (result, 0, 4, scale) == Catch::Approx (0.96078f));
                CHECK (getScaledBrightness (result, 1, 4, scale) == Catch::Approx (0.89804f));

                // top
                CHECK (getScaledBrightness (result, 4, 0, scale) == Catch::Approx (0.96078f));
                CHECK (getScaledBrightness (result, 4, 1, scale) == Catch::Approx (0.89804f));

                // right
                CHECK (getScaledBrightness (result, 8, 4, scale) == Catch::Approx (0.96078f));
                CHECK (getScaledBrightness (result, 7, 4, scale) == Catch::Approx (0.89804f));

                // bottom
                CHECK (getScaledBrightness (result, 4, 8, scale) == Catch::Approx (0.96078f));
                CHECK (getScaledBrightness (result, 4, 7, scale) == Catch::Approx (0.88235f));
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
            CHECK (result.getPixelAt (1, 4).getBrightness() == Catch::Approx (0.91373f).margin (0.05f));

            // top
            CHECK (result.getPixelAt (4, 0).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 1).getBrightness() == Catch::Approx (0.91373f).margin (0.05f));

            // right
            CHECK (result.getPixelAt (8, 4).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (7, 4).getBrightness() == Catch::Approx (0.91373f).margin (0.05f));

            // bottom
            CHECK (result.getPixelAt (4, 8).getBrightness() == Catch::Approx (1.0f));
            CHECK (result.getPixelAt (4, 7).getBrightness() == Catch::Approx (0.91373f).margin (0.05f));
        }
    }
}
