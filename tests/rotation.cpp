#include "../melatonin/support/rotation.h"
#include "catch2/matchers/catch_matchers.hpp"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

TEST_CASE ("Melatonin Blur Rotation")
{
    SECTION ("square")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 10, 10, true);
        juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);

        // for simplicity, we'll avoid padding with a different line stride here
        std::vector<uint8_t> rotated (image.getWidth() * image.getHeight());

        // needed for JUCE not to pee its pants (aka leak) when working with graphics
        juce::ScopedJuceInitialiser_GUI juce;
        juce::Graphics g (image);
        g.fillAll (juce::Colours::white);

        SECTION ("rotate (clockwise)")
        {
            SECTION ("top edge becomes right edge")
            {
                for (auto i = 0; i < image.getWidth(); i++)
                    data.setPixelColour (i, 0, juce::Colours::black);

                melatonin::rotateSingleChannel (image, rotated.data());

                // top edge now white
                for (auto i = 0; i < image.getWidth(); i++)
                    CHECK (rotated[i] == 255);

                // right edge black
                for (auto i = 0; i < image.getWidth(); i++)
                {
                    auto col = i * image.getWidth();
                    auto lastPixel = image.getWidth() - 1;
                    CHECK (rotated[col + lastPixel] == 255);
                }
            }

            SECTION ("left edge becomes top edge")
            {
                for (auto i = 0; i < image.getHeight(); i++)
                    data.setPixelColour (0, i, juce::Colours::black);

                melatonin::rotateSingleChannel (image, rotated.data());

                // left edge no longer black
                for (auto i = 0; i < image.getHeight(); i++)
                    CHECK (rotated[i * image.getWidth()] == 255);

                // top edge black
                for (auto i = 0; i < image.getWidth(); i++)
                    CHECK (rotated[i] == 255);
            }
        }

        SECTION ("unrotate (counterclockwise)")
        {
            SECTION ("rotate, then unrotate restores the top edge")
            {
                for (auto i = 0; i < image.getWidth(); i++)
                    data.setPixelColour (i, 0, juce::Colours::black);

                melatonin::rotateSingleChannel (image, rotated.data());

                // top edge of rotated is white
                for (auto i = 0; i < image.getWidth(); i++)
                    CHECK (rotated[i] == 255);

                melatonin::unrotateSingleChannel (rotated.data(), image);

                // top edge of image is back to black
                for (auto i = 0; i < image.getWidth(); i++)
                    CHECK (*data.getPixelPointer (i, 0) == 0);

                // right edge back to black
                for (auto i = 0; i < image.getWidth(); i++)
                {
                    auto col = i * data.lineStride; // important this is linestride, not width
                    auto lastPixel = image.getWidth() - 1;
                    CHECK (*data.getPixelPointer(lastPixel, col) == 0);
                }
            }
        }
    }
}
