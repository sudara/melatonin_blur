#include "catch2/benchmark/catch_benchmark.hpp"
#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"
#include "melatonin/implementations/gin.h"
#include "juce_graphics/juce_graphics.h"
#include "melatonin_blur/melatonin_blur.h"
#include "../melatonin/implementations/naive.h"
#include "../melatonin/implementations/float_vector_stack_blur.h"

// other benchmarks
#include "single_channel.cpp"
#include "argb.cpp"
#include "drop_shadow.cpp"

TEST_CASE ("Melatonin Blur Benchmarks Misc")
{
    // 100x100 white image with a 75x75 black square that will be blurred
    SECTION ("martin optimization 75x75px black on 100x100px white with 10px blur")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 100, 100, true);
        juce::Graphics g (image);
        g.setColour (juce::Colours::black);
        g.drawRect (25, 25, 75, 75);
        juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);

        BENCHMARK ("Gin")
        {
            melatonin::stackBlur::ginSingleChannel (image, 10);
            auto color = data.getPixelColour (20, 20);
            return color;
        };

//        BENCHMARK ("Naive")
//        {
//            melatonin::stackBlur::circularBufferSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
//
//        BENCHMARK ("With Martin Optimization")
//        {
//            melatonin::stackBlur::martinOptimizationSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
//
//        BENCHMARK ("templated float w/ martin")
//        {
//            melatonin::stackBlur::templatedFloatSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
    }

    SECTION ("martin optimization 50x50px black on 100x100px white with 10px blur")
    {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, 100, 100, true);
        juce::Graphics g (image);
        g.setColour (juce::Colours::black);
        g.drawRect (50, 50, 50, 50);
        juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);

//        BENCHMARK ("Naive")
//        {
//            melatonin::stackBlur::circularBufferSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
//
//        BENCHMARK ("With Martin Optimization")
//        {
//            melatonin::stackBlur::martinOptimizationSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
//
//        BENCHMARK ("templated float w/ martin")
//        {
//            melatonin::stackBlur::templatedFloatSingleChannel (image, 10);
//            auto color = data.getPixelColour (20, 20);
//            return color;
//        };
    }
}
