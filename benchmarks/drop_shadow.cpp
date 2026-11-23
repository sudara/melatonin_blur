TEST_CASE ("Melatonin Blur single drop shadow opacity benchmarks")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::NativeImageType native;
    const juce::SoftwareImageType software;
    const juce::ImageType* imageTypes[] { &native, &software };

    for (const auto* type : imageTypes)
    {
        for (int size : { 48, 256 })
        {
            DYNAMIC_SECTION ((type == &native ? "Native" : "Software") << " " << size << "px")
            {
                juce::Path path;
                path.addRoundedRectangle (20, 20, (float) size, (float) size, 5);
                juce::Image target (juce::Image::ARGB, size + 40, size + 40, true, *type);
                juce::Graphics g (target);
                melatonin::DropShadow shadow { { juce::Colours::blue.withAlpha (0.5f), 8 } };
                shadow.render (g, path);

                BENCHMARK ("Static 50% opacity")
                {
                    shadow.render (g, path);
                };

                bool high = false;
                BENCHMARK ("Alternating 25% and 75% opacity")
                {
                    high = ! high;
                    shadow.setOpacity (high ? 0.75f : 0.25f);
                    shadow.render (g, path);
                };
            }
        }
    }
}

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
