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
    auto bounds = juce::Rectangle<float> (3, 3);
    juce::Path p;
    p.addRectangle (bounds.translated (3, 3));

    juce::ScopedJuceInitialiser_GUI juce;

    juce::Image result (juce::Image::ARGB, 9, 9, true);

    SECTION ("fades reuse an opaque composite and static repaints use baked opacity")
    {
        struct Renderer : juce::LowLevelGraphicsSoftwareRenderer
        {
            using LowLevelGraphicsSoftwareRenderer::LowLevelGraphicsSoftwareRenderer;

            void drawImage (const juce::Image& image, const juce::AffineTransform& transform) override
            {
                compositeAlpha = image.getPixelAt (image.getWidth() / 2, image.getHeight() / 2).getAlpha();
                LowLevelGraphicsSoftwareRenderer::drawImage (image, transform);
            }

            int compositeAlpha = -1;
        };

        Renderer renderer (result);
        juce::Graphics g (renderer);
        melatonin::DropShadow shadow { { juce::Colours::black.withAlpha (0.5f), 1 } };
        auto checkDraw = [&] (int alpha, int compositeAlpha) {
            g.fillAll (juce::Colours::white);
            shadow.render (g, p);
            CHECK (renderer.compositeAlpha == compositeAlpha);
            CHECK (std::abs (result.getPixelAt (4, 4).getRed() - (255 - alpha)) <= 1);
        };

        checkDraw (128, 128);
        shadow.setOpacity (0.25f);
        checkDraw (64, 255);
        shadow.setColor (juce::Colours::black.withAlpha (0.75f));
        checkDraw (191, 255);
        checkDraw (191, 191);

        juce::Image native (juce::Image::ARGB, 9, 9, true, juce::NativeImageType());
        shadow.setOpacity (0.5f);
        render (shadow, native, p);
        CHECK (std::abs (native.getPixelAt (4, 4).getRed() - 127) <= 1);
        checkDraw (128, 128);
    }

    SECTION ("a fade frame and the steady repaint after it give the same pixels on a native image")
    {
        juce::Image native (juce::Image::ARGB, 9, 9, true, juce::NativeImageType());
        melatonin::DropShadow shadow { { juce::Colours::orange.withAlpha (0.5f), 1 } };
        render (shadow, native, p);

        for (const auto opacity : { 0.25f, 0.75f, 0.1f, 1.0f })
        {
            shadow.setOpacity (opacity);
            render (shadow, native, p);
            const auto fadeFrame = native.createCopy();
            render (shadow, native, p);

            for (int y = 0; y < native.getHeight(); ++y)
            {
                for (int x = 0; x < native.getWidth(); ++x)
                {
                    const auto fade = fadeFrame.getPixelAt (x, y);
                    const auto steady = native.getPixelAt (x, y);
                    CHECK (std::abs (fade.getRed() - steady.getRed()) <= 1);
                    CHECK (std::abs (fade.getGreen() - steady.getGreen()) <= 1);
                    CHECK (std::abs (fade.getBlue() - steady.getBlue()) <= 1);
                }
            }

            // orange has no blue, so blue at a fully covered pixel shows the alpha
            CHECK (std::abs (native.getPixelAt (4, 4).getBlue() - juce::roundToInt (255.0f * (1.0f - opacity))) <= 1);
        }
    }

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

        SECTION ("opacity")
        {
            SECTION ("setOpacity preserves the single-shadow cache")
            {
                shadow.setOpacity (0.5f);
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == false);
            }

            SECTION ("setColor preserves the single-shadow cache when only alpha changes")
            {
                shadow.setColor (juce::Colours::black.withAlpha (0.5f));
                CHECK (shadow.willRecalculate() == false);
                CHECK (shadow.willRecomposite() == false);
            }

            SECTION ("a cached black shadow fades to gray over white")
            {
                shadow.setOpacity (0.5f);
                render (shadow, result, p);

                const auto gray = result.getPixelAt (4, 4).getRed();
                CHECK (gray >= 126);
                CHECK (gray <= 129);

                shadow.setOpacity (1.0f);
                render (shadow, result, p);
                CHECK (getPixels (result, 4, { 4, 4 }) == "FF000000");
            }

            SECTION ("setColor updates both RGB and alpha")
            {
                shadow.setColor (juce::Colours::red.withAlpha (0.5f));
                CHECK (shadow.willRecomposite() == true);
                render (shadow, result, p);

                const auto pixel = result.getPixelAt (4, 4);
                CHECK (pixel.getRed() == 255);
                CHECK (pixel.getGreen() >= 126);
                CHECK (pixel.getGreen() <= 129);
                CHECK (pixel.getBlue() == pixel.getGreen());
            }

            SECTION ("changing one layer's opacity invalidates the composite")
            {
                melatonin::DropShadow two { { juce::Colours::black, 1 }, { juce::Colours::black, 2 } };
                render (two, result, p);

                two.setOpacity (0.5f);
                CHECK (two.willRecalculate() == false);
                CHECK (two.willRecomposite() == true);
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
                melatonin::DropShadow two { { juce::Colours::black, 1 }, { juce::Colours::black, 2 } };
                render (two, result, p);

                two.setOpacity (0.5f);
                two.setColor (juce::Colours::black.withAlpha (0.5f));
                CHECK (two.willRecalculate() == false);
                CHECK (two.willRecomposite() == true);
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
