TEST_CASE ("Melatonin Blur ARGB Benchmarks")
{
    for (auto dimension : { 50, 200, 500, 1000 })
    {
        DYNAMIC_SECTION ("ARGB, Image Size " << dimension << "x" << dimension)
        {
            juce::Image image (juce::Image::PixelFormat::ARGB, dimension, dimension, true);
            juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);
            juce::HeapBlock<char> heapBlock (dimension);

            for (auto radius : { 5, 10, 50 })
            {
                if (dimension < radius * 2 + 1)
                    continue;

                DYNAMIC_SECTION ("with radius " << radius)
                {
                    BENCHMARK ("gin")
                    {
                        melatonin::stackBlur::ginRGBA (image, radius);
                        auto color = data.getPixelColour (20, 20);
                        return color;
                    };

                    BENCHMARK ("Melatonin")
                    {
                        melatonin::blur::argb (image, radius);
                        auto color = data.getPixelColour (20, 20);
                        return color;
                    };

                    // pretty much windows only
                    BENCHMARK ("Float Vector")
                    {
                        melatonin::blur::juceFloatVectorARGB (image, radius);
                        auto color = data.getPixelColour (20, 20);
                        return color;
                    };
                }
            }
        }
    }
}
