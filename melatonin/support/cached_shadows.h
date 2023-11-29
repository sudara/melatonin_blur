#pragma once
#include "helpers.h"

namespace melatonin
{
    // This class isn't meant for direct usage and may change its API over time!
    // Use DropShadow and InnerShadow and PathWithShadows instead, you've been warned...
    class CachedShadows
    {
    protected:
        std::vector<ShadowParameters> shadowParameters;

        CachedShadows (std::initializer_list<ShadowParameters> p, bool inner = false)
            : shadowParameters (p)
        {
            jassert (!shadowParameters.empty());

            renderedShadows.resize (shadowParameters.size());

            // allow specifying of inner shadows in bulk
            if (inner)
                std::for_each (shadowParameters.begin(), shadowParameters.end(), [] (auto& s) { s.inner = true; });
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
                recalculateBlurs (g);
            }

            drawCachedBlurs (g, optimizeClipBounds);
        }

    private:
        juce::Path path;
        std::vector<juce::Image> renderedShadows;

        void recalculateBlurs (juce::Graphics& g)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];
                if (s.radius < 1)
                    continue;

                renderedShadows[i] = renderShadowToARGB (s, path, g.getInternalContext().getPhysicalPixelScaleFactor());
            }
        }

        void drawCachedBlurs (juce::Graphics& g, bool optimizeClipBounds = false)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];
                float scale = 1.0;
                if (!s.lowQuality)
                    scale = g.getInternalContext().getPhysicalPixelScaleFactor();

                // needed to allow this to support 0 radius blurs (for animation, etc)
                if (s.radius < 1)
                    continue;

                // resets the Clip Region when this scope ends
                juce::Graphics::ScopedSaveState saveState (g);

                // for inner shadows we don't want anything outside the path bounds
                if (s.inner)
                    g.reduceClipRegion ((path.getBounds() * scale).getSmallestIntegerContainer());

                // TODO: requires testing/benchmarking
                else if (optimizeClipBounds)
                {
                    // don't bother drawing what's inside the path's bounds
                    g.excludeClipRegion (path.getBounds().toNearestIntEdges());
                }

                // TODO: consider setting opacity here, not in the shadow rendering
                // That would let us cheaply do things like fade shadows in without rerendering!
                g.setColour (s.color);

                // `s.area` has been scaled by the physical pixel scale factor
                // (unless lowQuality is true)
                // we have to pass a 1/scale transform because the context will otherwise try to scale the image up
                // (which is not what we want, at this point we are 1:1)
                g.drawImageTransformed (renderedShadows[i], juce::AffineTransform::translation (s.area.getX(), s.area.getY()).scaled(1/scale));
            }
        }
    };
}
