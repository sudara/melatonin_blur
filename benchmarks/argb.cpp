TEST_CASE ("Melatonin Blur ARGB Benchmarks")
{
    for (auto dimension : { 50, 200, 500, 1000 })
    {
        DYNAMIC_SECTION ("ARGB, Image Size " << dimension << "x" << dimension)
        {
            juce::Image src (juce::Image::PixelFormat::ARGB, dimension, dimension, true);
            juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);

            juce::Image dst (juce::Image::PixelFormat::ARGB, dimension, dimension, true);
            juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readOnly);

            for (auto radius : { 8, 16, 32 })
            {
                if (dimension < radius * 2 + 1)
                    continue;

                DYNAMIC_SECTION ("with radius " << radius)
                {
                    melatonin::CachedBlur blur (radius);
                    blur.render (src);

                    BENCHMARK ("gin")
                    {
                        // this modifies the image directly!
                        melatonin::stackBlur::ginRGBA (src, radius);
                        auto color = srcData.getPixelColour (20, 20);
                        return color;
                    };

                    BENCHMARK ("Melatonin")
                    {
                        melatonin::blur::argb (src, dst, radius);
                        auto color = dstData.getPixelColour (20, 20);
                        return color;
                    };

                    // pretty much windows only
                    BENCHMARK ("Float Vector")
                    {
                        melatonin::blur::juceFloatVectorARGB (dst, radius);
                        auto color = dstData.getPixelColour (20, 20);
                        return color;
                    };
                }
            }
        }
    }
}
