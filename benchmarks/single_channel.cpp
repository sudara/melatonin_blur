TEST_CASE ("Melatonin Blur Single Channel Benchmarks")
{
    auto createSource = [] (int dimension) {
        juce::Image image (juce::Image::PixelFormat::SingleChannel, dimension, dimension, true);
        juce::Graphics g (image);
        g.setColour (juce::Colours::white);
        g.fillRect (dimension / 4, dimension / 4, dimension / 2, dimension / 2);
        return image;
    };

    for (auto dimension : { 50, 100, 200, 500, 1000 })
    {
        DYNAMIC_SECTION ("Single Channel, Image Size " << dimension << "x" << dimension)
        {
            juce::ScopedJuceInitialiser_GUI juce;

            for (auto radius : { 5, 10, 15, 20, 50 })
            {
                if (dimension < radius * 2 + 1)
                    continue;

                DYNAMIC_SECTION ("with radius " << radius)
                {
                    auto ginImage = createSource (dimension);
                    auto naiveImage = createSource (dimension);
                    auto floatVectorImage = createSource (dimension);

                   #if MELATONIN_BLUR_USE_DIRECT2D
                    auto cpuFallbackImage = createSource (dimension);
                    auto direct2DSrc = createSource (dimension);
                    auto direct2DDst = createSource (dimension);
                    melatonin::blur::Direct2DSingleChannelBlur direct2DBlur;
                    direct2DBlur.render (direct2DSrc, direct2DDst, (size_t) radius);
                   #else
                    auto melatoninImage = createSource (dimension);
                   #endif

                    //                    BENCHMARK ("dequeue")
                    //                    {
                    //                        melatonin::stackBlur::dequeueSingleChannel (image, radius);
                    //                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                    //                        return color;
                    //                    };
                    //

                    BENCHMARK ("Gin (reference implementation)")
                    {
                        melatonin::stackBlur::ginSingleChannel (ginImage, radius);
                        return ginImage.getWidth();
                    };

                    BENCHMARK ("Naive (Circular Buffer)")
                    {
                        melatonin::stackBlur::circularBufferSingleChannel (naiveImage, radius);
                        return naiveImage.getWidth();
                    };

                    //                    BENCHMARK ("Tent")
                    //                    {
                    //                        melatonin::stackBlur::tentBlurSingleChannel (image, radius);
                    //                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                    //                        return color;
                    //                    };

                    //                    BENCHMARK ("templated function")
                    //                    {
                    //                        melatonin::stackBlur::singleChannelTemplated (image, radius);
                    //                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                    //                        return color;
                    //                    };

//                    BENCHMARK ("templated function float")
//                    {
//                        melatonin::stackBlur::templatedFloatSingleChannel (image, radius);
//                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
//                        return color;
//                    };
//
//                    BENCHMARK ("melatonin vector")
//                    {
//                        melatonin::stackBlur::vectorSingleChannel (image, radius);
//                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
//                        return color;
//                    };
                    //                    BENCHMARK ("vector class")
                    //                    {
                    //                        melatonin::VectorStackBlur stackBlur (image, radius);
                    //                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                    //                        return color;
                    //                    };

                    BENCHMARK ("JUCE FloatVectorOperations")
                    {
                        melatonin::blur::juceFloatVectorSingleChannel (floatVectorImage, radius);
                        return floatVectorImage.getWidth();
                    };

                   #if MELATONIN_BLUR_USE_DIRECT2D
                    BENCHMARK_ADVANCED ("Melatonin CPU fallback (D2D runtime OFF)") (Catch::Benchmark::Chronometer meter)
                    {
                        const ScopedDirect2DBenchmarkSetting mode (false);

                        meter.measure ([&] {
                            melatonin::blur::cpuSingleChannel (cpuFallbackImage, radius);
                            return cpuFallbackImage.getWidth();
                        });
                    };

                    BENCHMARK_ADVANCED ("Direct2D source-to-dest (reused renderer)") (Catch::Benchmark::Chronometer meter)
                    {
                        const ScopedDirect2DBenchmarkSetting mode (true);

                        meter.measure ([&] {
                            direct2DBlur.render (direct2DSrc, direct2DDst, (size_t) radius);
                            return direct2DDst.getWidth();
                        });
                    };
                   #else
                    BENCHMARK ("Melatonin")
                    {
                        melatonin::blur::singleChannel (melatoninImage, radius);
                        return melatoninImage.getWidth();
                    };
                   #endif
                }
            }
        }
    }
}
