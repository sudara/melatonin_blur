#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// this JUCE stuff is low level, not well documented as to its purpose, nor tested by JUCE?
// but there's a good example in StandardCachedComponentImage in juce_Component.cpp
// That uses g2.getInternalContext() to then add a transform to the internal context
// but we can just also use g2.addTransform (maybe added later?)
TEST_CASE ("Melatonin Blur JUCE sanity checks")
{
    SECTION ("path drawing respects context scale")
    {
        auto scaleFactor = 2.0f;

        // the image bounds backing our temporary context must be able to contain the transformed image
        // here we will work in 4x4, it will be transformed by the context to 8x8
        juce::Image image (juce::Image::PixelFormat::ARGB, 8, 8, true);
        juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);

        // create our new context
        juce::Graphics g2 (image);
        g2.addTransform (juce::AffineTransform::scale (scaleFactor));

        // create a 4x4 rectangle
        juce::Path p;
        p.addRectangle (juce::Rectangle<float> (4, 4));

        // draw the rectangle
        g2.setColour (juce::Colours::lime);
        g2.fillPath (p);

        save_test_image(image, "path_drawing_respects_context_scale");

        CHECK(image.getPixelAt(0, 0).toString() == juce::Colours::lime.toString());
        CHECK(data.getPixelColour (0,0) == juce::Colours::lime);
        CHECK(data.getPixelColour (0,0).toString() == juce::Colours::lime.toString());


        // confirm the entire 8x8 is filled
        CHECK (isImageFilled (image, juce::Colours::lime) == true);
    }

    // this is relevant to our "lowQuality" setting
    // anyone using stackblur is probably doing low quality blurs without realizing it
    SECTION ("drawImageAt upscales when context scale is larger than image scale")
    {
        auto scaleFactor = 2.0f;

        // the image bounds backing our temporary context must be able to contain the transformed image
        // our source will be 4x4, transformed by the context to 8x8
        juce::Image simulated2xContext (juce::Image::PixelFormat::ARGB, 8, 8, true);

        juce::Graphics g (simulated2xContext);
        g.addTransform (juce::AffineTransform::scale (scaleFactor));

        // simulate creating a normal 4x4 image
        // for example, our "lowQuality" drop shadows do this behind the scenes
        juce::Image source (juce::Image::PixelFormat::ARGB, 4, 4, true);
        juce::Graphics g2 (source);
        g2.setColour (juce::Colours::lime);
        g2.fillAll();

        g.drawImageAt (source, 0, 0, false);

        // confirm the entire 8x8 is filled
        CHECK (isImageFilled (simulated2xContext, juce::Colours::lime) == true);
    }

    SECTION ("drawImage at where source and target both @2x")
    {
        auto scaleFactor = 2.0f;

        // the image bounds backing our temporary context must be able to contain the transformed image
        // our source will be 4x4, transformed by the context to 8x8
        juce::Image simulated2xContext (juce::Image::PixelFormat::ARGB, 8, 8, true);

        juce::Graphics g (simulated2xContext);
        g.addTransform (juce::AffineTransform::scale (scaleFactor));

        // simulate creating a 8x8 image, like our high quality blurs do
        juce::Image source (juce::Image::PixelFormat::ARGB, 8, 8, true);
        juce::Graphics g2 (source);

        // the transforms on source and target match
        g2.addTransform (juce::AffineTransform::scale (scaleFactor));
        g2.setColour (juce::Colours::lime);
        g2.fillAll();

        // TODO: I wonder if JUCE's drawImageAt could be cheaper if the transforms were compared?
        g.drawImageAt (source, 0, 0, false);

        // confirm the entire 8x8 is filled
        CHECK (isImageFilled (simulated2xContext, juce::Colours::lime) == true);
    }
}
