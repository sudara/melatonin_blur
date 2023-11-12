TEST_CASE ("Melatonin Blur Single Channel Benchmarks")
{
    for (auto dimension : { 50, 100, 200, 500, 1000 })
    {
        DYNAMIC_SECTION ("Single Channel, Image Size " << dimension << "x" << dimension)
        {
            juce::Image image (juce::Image::PixelFormat::SingleChannel, dimension, dimension, true);
            juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);

            for (auto radius : { 5, 10, 15, 20, 50 })
            {
                if (dimension < radius * 2 + 1)
                    continue;

                DYNAMIC_SECTION ("with radius " << radius)
                {
                    //                    BENCHMARK ("dequeue")
                    //                    {
                    //                        melatonin::stackBlur::dequeueSingleChannel (image, radius);
                    //                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                    //                        return color;
                    //                    };
                    //

                    BENCHMARK ("Gin (reference implementation)")
                    {
                        melatonin::stackBlur::ginSingleChannel (image, radius);
                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                        return color;
                    };

                    BENCHMARK ("Naive (Circular Buffer)")
                    {
                        melatonin::stackBlur::circularBufferSingleChannel (image, radius);
                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                        return color;
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
                        melatonin::blur::juceFloatVectorSingleChannel (image, radius);
                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                        return color;
                    };

                    BENCHMARK ("Melatonin")
                    {
                        melatonin::blur::singleChannel (image, radius);
                        auto color = data.getPixelColour (dimension - radius, dimension - radius);
                        return color;
                    };
                }
            }
        }
    }
}
