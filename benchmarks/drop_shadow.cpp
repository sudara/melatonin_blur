TEST_CASE ("Melatonin Blur Drop Shadow Benchmarks")
{
    for (auto dimension : { 20, 50, 100, 500 })
    {
        DYNAMIC_SECTION ("Path Size:" << dimension << "x" << dimension)
        {
            juce::Path p;
            // 20x20 px rectangle in a 100x100 px image
            p.addRectangle (dimension / 2, dimension / 2, dimension, dimension);

            // typical shadow example
            melatonin::DropShadow shadow = {
                { juce::Colours::red, 48, { 2, 2 } },
                { juce::Colours::black, 36, { 0, 8 } }
            };
            juce::Image image (juce::Image::PixelFormat::ARGB, dimension * 2, dimension * 2, true);

            // needed for JUCE not to pee its pants (aka leak) when working with graphics
            juce::ScopedJuceInitialiser_GUI juce;
            juce::Graphics g (image);
            g.fillAll (juce::Colours::white);

            SECTION ("single render")
            {
                shadow.render (g, p);
                BENCHMARK ("Reference (gin)")
                {
                    melatonin::stackBlur::renderDropShadow (g, p, juce::Colours::red, 48, { 2, 2 });
                    melatonin::stackBlur::renderDropShadow (g, p, juce::Colours::black, 36, { 0, 8 });
                    return image.getPixelAt (20, 20);
                };

                BENCHMARK ("Melatonin (cached)")
                {
                    shadow.render (g, p);
                    return image.getPixelAt (20, 20);
                };
            }
        }
    }
}
