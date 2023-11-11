#pragma once
#include "helpers.h"

namespace melatonin
{
    // This class isn't meant for direct usage!
    // Use DropShadow and InnerShadow
    class CachedShadow
    {
    protected:
        std::vector<ShadowParameters> shadowParameters;

        CachedShadow (std::initializer_list<ShadowParameters> p)
            : shadowParameters (p)
        {
            jassert (!shadowParameters.empty());

            for (auto& shadow : shadowParameters)
            {
                // 0 radius means no shadow..
                if (shadow.radius < 1)
                {
                    jassertfalse;
                    continue;
                }

                // each shadow is backed by a JUCE image
                renderedShadows.emplace_back();
            }
        }

    public:
        void render (juce::Graphics& g, const juce::Path& newPath, bool optimizeClipBounds = false)
        {
            // recalculate blurs only when the path changes (otherwise render from cache)
            // TODO: The path is stored in this class, probably not necessary/efficient
            // Can it be replaced with a hashing mechanism?
            if (newPath != path)
            {
                path = newPath;
                recalculateBlurs();
            }

            drawCachedBlurs (g, optimizeClipBounds);
        }


    private:
        juce::Path path;
        std::vector<juce::Image> renderedShadows;

        void recalculateBlurs ()
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];
                renderedShadows[i] = renderShadowToARGB (s, path);
            }
        }

        void drawCachedBlurs (juce::Graphics& g, bool optimizeClipBounds = false)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];

                // resets the Clip Region when this scope ends
                juce::Graphics::ScopedSaveState saveState (g);

                // for inner shadows we don't want anything outside the path bounds
                if (s.inner)
                    g.reduceClipRegion (path);
                else if (optimizeClipBounds)
                {
                    // don't bother drawing what's inside the path's bounds
                    // TODO: requires testing/benchmarking
                    g.excludeClipRegion (path.getBounds().toNearestIntEdges());
                }

                // Not sure why, but this is required despite fillAlphaChannelWithCurrentBrush=false
                g.setColour (s.color);

                // Specifying `false` for `fillAlphaChannelWithCurrentBrush` here
                // is a 2-3x speedup on the actual image rendering
                g.drawImageAt (renderedShadows[i], s.area.getX(), s.area.getY());
            }
        }
    };
}
