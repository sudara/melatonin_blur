TEST_CASE ("Melatonin Blur Drop Shadow Benchmarks")
{
    for (auto dimension : { 20, 50, 100, 500 })
    {
        DYNAMIC_SECTION ("Path Size:" << dimension << "x" << dimension)
        {
            juce::Path p;
            // 20x20 px rectangle in a 100x100 px image
            const auto pathSize = static_cast<float> (dimension);
            p.addRectangle (pathSize / 2.0f, pathSize / 2.0f, pathSize, pathSize);

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

               #if MELATONIN_BLUR_USE_DIRECT2D
                juce::Path alternatePath;
                alternatePath.addEllipse (pathSize / 2.0f, pathSize / 2.0f, pathSize, pathSize);

                melatonin::DropShadow cpuShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                melatonin::DropShadow direct2DShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                melatonin::DropShadow cpuRecalculatedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };
                melatonin::DropShadow direct2DRecalculatedShadow = {
                    { juce::Colours::red, 48, { 2, 2 } },
                    { juce::Colours::black, 36, { 0, 8 } }
                };

                BENCHMARK_ADVANCED ("Melatonin (cached draw only, D2D runtime OFF / CPU)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (false);

                    meter.measure ([&] {
                        cpuShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin (cached draw only, D2D runtime ON)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (true);

                    meter.measure ([&] {
                        direct2DShadow.render (g, p);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin (rebuild blur, D2D runtime OFF / CPU)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (false);
                    auto useAlternatePath = false;

                    meter.measure ([&] {
                        useAlternatePath = ! useAlternatePath;
                        cpuRecalculatedShadow.render (g, useAlternatePath ? p : alternatePath);
                        return image.getPixelAt (20, 20);
                    });
                };

                BENCHMARK_ADVANCED ("Melatonin (rebuild blur, D2D runtime ON)") (Catch::Benchmark::Chronometer meter)
                {
                    const ScopedDirect2DBenchmarkSetting mode (true);
                    auto useAlternatePath = false;

                    meter.measure ([&] {
                        useAlternatePath = ! useAlternatePath;
                        direct2DRecalculatedShadow.render (g, useAlternatePath ? p : alternatePath);
                        return image.getPixelAt (20, 20);
                    });
                };
               #else
                BENCHMARK ("Melatonin (cached)")
                {
                    shadow.render (g, p);
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
