TEST_CASE ("Melatonin Blur ARGB Benchmarks")
{
    for (auto dimension : { 50, 200, 500, 1000 })
    {
        DYNAMIC_SECTION ("ARGB, Image Size " << dimension << "x" << dimension)
        {
            juce::Image context (juce::Image::PixelFormat::ARGB, dimension, dimension, true);

            juce::Image src (juce::Image::PixelFormat::ARGB, dimension, dimension, true);
            juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);

            juce::Image dst (juce::Image::PixelFormat::ARGB, dimension, dimension, true);
            juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readOnly);

            juce::ScopedJuceInitialiser_GUI juce;
            juce::Graphics g (context);
            g.fillAll (juce::Colours::white);

            for (auto radius : { 8, 16, 32, 48 })
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
                        melatonin::stackBlur::ginARGB (src, radius);
                        g.drawImageAt (src, 0, 0, true);
                        auto color = srcData.getPixelColour (20, 20);
                        return color;
                    };

                    BENCHMARK ("Melatonin uncached")
                    {
                        // uses a temp copy internally
                        melatonin::blur::argb (src, dst, radius);
                        g.drawImageAt (src, 0, 0, true);
                        auto color = dstData.getPixelColour (20, 20);
                        return color;
                    };

                    // pretty much windows only
                    BENCHMARK ("Float Vector")
                    {
                        melatonin::blur::juceFloatVectorARGB (dst, radius);
                        g.drawImageAt (dst, 0, 0, true);
                        auto color = dstData.getPixelColour (20, 20);
                        return color;
                    };

                    BENCHMARK ("Melatonin Cached")
                    {
                        // returns a juce::Image to render
                        g.drawImageAt (blur.render (src), 0, 0, true);
                        auto color = dstData.getPixelColour (20, 20);
                        return color;
                    };
                }
            }
        }
    }
}
