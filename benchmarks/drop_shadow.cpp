TEST_CASE ("Melatonin Blur Drop Shadow Benchmarks")
{
    for (auto dimension : { 20, 50, 100, 500 })
    {
        DYNAMIC_SECTION ("Path Size:" << dimension << "x" << dimension)
        {
            juce::Path p;
            const auto pathSize = static_cast<float> (dimension);
            p.addRectangle (pathSize / 2.0f, pathSize / 2.0f, pathSize, pathSize);

            juce::Image image (juce::Image::PixelFormat::ARGB, dimension * 2, dimension * 2, true);

            // needed for JUCE not to pee its pants (aka leak) when working with graphics
            juce::ScopedJuceInitialiser_GUI juce;
            juce::Graphics g (image);
            g.fillAll (juce::Colours::white);

            SECTION ("single render")
            {
                BENCHMARK ("Reference (gin)")
                {
                    melatonin::stackBlur::renderDropShadow (g, p, juce::Colours::red, 48, { 2, 2 });
                    melatonin::stackBlur::renderDropShadow (g, p, juce::Colours::black, 36, { 0, 8 });
                    return image.getPixelAt (20, 20);
                };

                melatonin::DropShadow cachedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                melatonin::DropShadow uncachedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                uncachedShadow.setBypassCache (true);

               #if MELATONIN_BLUR_USE_DIRECT2D
                melatonin::DropShadow cpuCachedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                melatonin::DropShadow cpuUncachedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                cpuUncachedShadow.setBypassCache (true);

                BENCHMARK_ADVANCED ("Melatonin (cached)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (true);

                    meter.measure ([&] {
                        cachedShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin (uncached)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (true);

                    meter.measure ([&] {
                        uncachedShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin D2D OFF / CPU fallback (cached)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (false);

                    meter.measure ([&] {
                        cpuCachedShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin D2D OFF / CPU fallback (uncached)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (false);

                    meter.measure ([&] {
                        cpuUncachedShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };
               #else
                BENCHMARK ("Melatonin (cached)")
                {
                    cachedShadow.render (g, p);
                    return image.getPixelAt (20, 20);
                };

                BENCHMARK ("Melatonin (uncached)")
                {
                    uncachedShadow.render (g, p);
                    return image.getPixelAt (20, 20);
                };
               #endif

                BENCHMARK ("juce")
                {
                    juce::DropShadow(juce::Colours::red, 48, { 2, 2 }).drawForPath (g, p);
                    juce::DropShadow(juce::Colours::black, 36, { 0, 8 }).drawForPath (g, p);
                };
            }
        }
    }
}
