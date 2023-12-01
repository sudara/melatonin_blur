#pragma once
#include "rendering.h"

namespace melatonin::internal
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

            renderedSingleChannelShadows.resize (shadowParameters.size());

            // allow specifying of inner shadows in bulk
            if (inner)
                std::for_each (shadowParameters.begin(), shadowParameters.end(), [] (auto& s) { s.inner = true; });
        }

    public:
        void render (juce::Graphics& g, const juce::Path& newPath, bool lowQuality = false)
        {
            // Before Melatonin Blur, it was all low quality!
            float scale = 1.0;
            if (!lowQuality)
                scale = g.getInternalContext().getPhysicalPixelScaleFactor();

            // store a copy of the new path, stripping and storing its float x/y offset to 0,0
            auto incomingOrigin = newPath.getBounds().getPosition();
            auto incomingOriginAgnosticPath = newPath;
            incomingOriginAgnosticPath.applyTransform (juce::AffineTransform::translation (-incomingOrigin));

            // has the path actually changed?
            if (incomingOriginAgnosticPath != lastOriginAgnosticPath)
            {
                // we already created a copy above, this is faster than creating another
                lastOriginAgnosticPath.swapWithPath (incomingOriginAgnosticPath);

                // remember the new placement in the context
                pathPositionInContext = incomingOrigin;

                recalculateBlurs (scale);
            }

            // the path is the same, but it's been moved to new coordinates
            else if (incomingOrigin != pathPositionInContext)
            {
                // just reposition the cached shadows
                pathPositionInContext = incomingOrigin;
            }

            // have any of the shadows changed color/opacity or been recalculated?
            if (needsRecomposite)
                compositeShadowsToARGB (scale);

            drawARGBComposite (g, scale);
        }

    private:
        // store a copy of the path to compare against for caching
        juce::Path lastOriginAgnosticPath = {};

        // any float offset from 0,0 the path has is stored here
        juce::Point<float> pathPositionInContext = {};

        // this stores the final, end result
        juce::Image compositedARGB;
        juce::Point<float> compositedARGBPositionInContext;

        // each component blur is stored here, their positions are stored in ShadowParameters
        std::vector<juce::Image> renderedSingleChannelShadows;

        // this lets us adjust color/opacity without re-rendering blurs
        bool needsRecomposite = true;

        void recalculateBlurs (float scale)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];
                if (s.radius < 1)
                    continue;

                renderedSingleChannelShadows[i] = renderShadowToSingleChannel (s, lastOriginAgnosticPath, scale);
                needsRecomposite = true;
            }
        }

        void drawARGBComposite (juce::Graphics& g, float scale, bool optimizeClipBounds = false)
        {
            // TODO: requires testing/benchmarking
            if (optimizeClipBounds)
            {
                // don't bother drawing what's inside the path's bounds
                g.excludeClipRegion (lastOriginAgnosticPath.getBounds().toNearestIntEdges());
            }

            // TODO: consider setting opacity here, not in the shadow rendering
            // That would let us cheaply do things like fade shadows in without rerendering!
            g.setOpacity (1.0);

            // `s.area` has been scaled by the physical pixel scale factor
            // (unless lowQuality is true)
            // we have to pass a 1/scale transform because the context will otherwise try to scale the image up
            // (which is not what we want, at this point our cached shadow is 1:1 with the context)
            auto position = (compositedARGBPositionInContext) + pathPositionInContext * scale;
            g.drawImageTransformed (compositedARGB, juce::AffineTransform::translation (position.getX(), position.getY()).scaled (1.0f / scale));
        }

        // This is done at the main graphics context scale
        // The path is at 0,0 and the shadows are placed at their correct relative *integer* positions
        void compositeShadowsToARGB (float scale)
        {
            // figure out the largest bounds we need to composite
            // this is the union of all the shadow bounds
            juce::Rectangle<int> unionOfAllBounds;
            for (auto& s : shadowParameters)
                unionOfAllBounds = unionOfAllBounds.getUnion (s.blurContextBoundsScaled);

            compositedARGBPositionInContext = unionOfAllBounds.getPosition().toFloat();

            // YET ANOTHER graphics context to efficiently convert the image to ARGB
            // why? Because later, compositing to the main graphics context (g) is faster
            // (won't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
            // which slows down the main compositing by a factor of 2-3x)
            // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76
            compositedARGB = { juce::Image::ARGB, (int) unionOfAllBounds.getWidth(), (int) unionOfAllBounds.getHeight(), true };

            // we already scaled up (if needed) the last round, no .addTransform here
            juce::Graphics g2 (compositedARGB);

            for (auto i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];

                auto offsetFromUnionBounds = s.blurContextBoundsScaled.getPosition() - unionOfAllBounds.getPosition();

                // support 0 radius blurs (for animation, etc)
                if (s.radius < 1)
                    continue;

                g2.setColour (s.color);

                // lets us temporarily clip the region if needed
                juce::Graphics::ScopedSaveState saveState (g2);

                // for inner shadows, don't draw anything outside the path bounds
                // ideally this would be done in compositing, but we'd have to pass the shadowPath bounds
                if (s.inner)
                {
                    auto scaledOriginAgnosticPathBounds = (lastOriginAgnosticPath.getBounds() * scale).getSmallestIntegerContainer();

                    // TODO: float is rounded here?...
                    auto scaledPathBounds = scaledOriginAgnosticPathBounds.translated((int) pathPositionInContext.getX(), (int) pathPositionInContext.getY());
                    auto pathInComposite = scaledPathBounds.translated (offsetFromUnionBounds.getX(), offsetFromUnionBounds.getY());
                    g2.reduceClipRegion (pathInComposite);
                }

                // the "true" means "fill the alpha channel with the current brush" — aka g's color
                g2.drawImageAt (renderedSingleChannelShadows[i], offsetFromUnionBounds.getX(), offsetFromUnionBounds.getY(), true);
            }
            save_test_image(compositedARGB, "argb composite");
            needsRecomposite = false;
        }
    };
}
