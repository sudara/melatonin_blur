#pragma once
#include "rendered_single_channel_shadow.h"

namespace melatonin::internal
{
    // This class isn't meant for direct usage and may change its API over time!
    // Use DropShadow and InnerShadow and PathWithShadows instead, you've been warned...
    class CachedShadows
    {
    protected:
        CachedShadows (std::initializer_list<ShadowParameters> shadowParameters, bool force_inner = false)
        {
            // gotta feed some shadows!
            jassert (shadowParameters.size() > 0);

            for (auto& parameters : shadowParameters)
            {
                auto& shadow = renderedSingleChannelShadows.emplace_back (parameters);

                if (force_inner)
                    shadow.parameters.inner = true;
            }
        }

    public:
        // store a copy of the path to compare against for caching
        // public for testability, sorry not sorry
        // too lazy to break out ARGBComposite into its own class
        juce::Path lastOriginAgnosticPath = {};

        void render (juce::Graphics& g, const juce::Path& newPath, bool lowQuality = false)
        {
            // Before Melatonin Blur, it was all low quality!
            float scale = 1.0;
            if (!lowQuality)
                scale = g.getInternalContext().getPhysicalPixelScaleFactor();

            if (!juce::approximatelyEqual (lastScale, scale))
            {
                needsRecalculate = true;
                lastScale = scale;
            }

            // store a copy of the new path, stripping and storing its float x/y offset to 0,0
            // stripping the origin lets us translate paths without breaking blur cache
            auto incomingOrigin = newPath.getBounds().getPosition();
            auto incomingOriginAgnosticPath = newPath;
            incomingOriginAgnosticPath.applyTransform (juce::AffineTransform::translation (-incomingOrigin));

            // has the path actually changed?
            if (needsRecalculate || (incomingOriginAgnosticPath != lastOriginAgnosticPath))
            {
                // we already created a copy above, this is faster than creating another
                lastOriginAgnosticPath.swapWithPath (incomingOriginAgnosticPath);

                // remember the new placement in the context
                pathPositionInContext = incomingOrigin;

                // create the single channel shadows
                recalculateBlurs (scale);
            }

            // the path is the same, but it's been moved to new coordinates
            else if (incomingOrigin != pathPositionInContext)
            {
                // reposition the cached single channel shadows
                pathPositionInContext = incomingOrigin;
            }

            // have any of the shadows changed color/opacity or been recalculated?
            // if so, recreate the ARGB composite of all the shadows together
            if (needsRecomposite)
                compositeShadowsToARGB (scale);

            // finally, draw the cached composite into the main graphics context
            drawARGBComposite (g, scale);
        }

        void renderStroked (juce::Graphics& g, const juce::Path& newPath, const juce::PathStrokeType& newType, bool lowQuality = false)
        {
            stroke = true;
            if (newType != strokeType)
            {
                strokeType = newType;
                needsRecalculate = true;
            }
            render (g, newPath, lowQuality);
        }

        void setRadius (size_t radius, size_t index = 0)
        {
            if (index < renderedSingleChannelShadows.size())
                needsRecalculate = renderedSingleChannelShadows[index].updateRadius ((int) radius);
        }

        void setSpread (size_t spread, size_t index = 0)
        {
            if (index < renderedSingleChannelShadows.size())
                needsRecalculate = renderedSingleChannelShadows[index].updateSpread ((int) spread);
        }

        void setOffset (juce::Point<int> offset, size_t index = 0)
        {
            if (index < renderedSingleChannelShadows.size())
                needsRecomposite = renderedSingleChannelShadows[index].updateOffset (offset, lastScale);
        }

        void setColor (juce::Colour color, size_t index = 0)
        {
            if (index < renderedSingleChannelShadows.size())
                needsRecomposite = renderedSingleChannelShadows[index].updateColor (color);
        }

        void setOpacity (float opacity, size_t index = 0)
        {
            if (index < renderedSingleChannelShadows.size())
                needsRecomposite = renderedSingleChannelShadows[index].updateOpacity (opacity);
        }

    private:
        // any float offset from 0,0 the path has is stored here
        juce::Point<float> pathPositionInContext = {};

        // this stores the final, end result
        juce::Image compositedARGB;
        juce::Point<float> scaledCompositePosition;

        // each component blur is stored here, their positions are stored in ShadowParameters
        std::vector<RenderedSingleChannelShadow> renderedSingleChannelShadows;

        // this lets us adjust color/opacity without re-rendering blurs
        bool needsRecomposite = true;
        bool needsRecalculate = true;

        float lastScale = 1.0;

        bool stroke = false;
        juce::PathStrokeType strokeType { -1.0f };

        void recalculateBlurs (float scale)
        {
            for (auto& shadow : renderedSingleChannelShadows)
            {
                shadow.render (lastOriginAgnosticPath, scale, strokeType);
            }
            needsRecalculate = false;
            needsRecomposite = true;
        }

        void drawARGBComposite (juce::Graphics& g, float scale, bool optimizeClipBounds = false)
        {
            // support default constructors, 0 radius blurs, etc
            if (compositedARGB.isNull())
                return;

            // resets the Clip Region when this scope ends
            juce::Graphics::ScopedSaveState saveState (g);

            // TODO: requires testing/benchmarking
            if (optimizeClipBounds)
            {
                // don't bother drawing what's inside the path's bounds
                g.excludeClipRegion (lastOriginAgnosticPath.getBounds().toNearestIntEdges());
            }

            // draw the composite at full strength
            // (the composite itself has the colors/opacity/etc)
            g.setOpacity (1.0);

            // `s.area` has been scaled by the physical pixel scale factor
            // (unless lowQuality is true)
            // we have to pass a 1/scale transform because the context will otherwise try to scale the image up
            // (which is not what we want, at this point our cached shadow is 1:1 with the context)
            auto position = (scaledCompositePosition) + (pathPositionInContext * scale);
            g.drawImageTransformed (compositedARGB, juce::AffineTransform::translation (position).scaled (1.0f / scale));
        }

        // This is done at the main graphics context scale
        // The path is at 0,0 and the shadows are placed at their correct relative *integer* positions
        void compositeShadowsToARGB (float scale)
        {
            // figure out the largest bounds we need to composite
            // this is the union of all the shadow bounds
            // they should all align with the path at 0,0
            juce::Rectangle<int> compositeBounds = {};
            for (auto& s : renderedSingleChannelShadows)
            {
                if (s.parameters.inner)
                    compositeBounds = compositeBounds.getUnion (s.getScaledPathBounds());
                else
                    compositeBounds = compositeBounds.getUnion (s.getScaledBounds());
            }

            scaledCompositePosition = compositeBounds.getPosition().toFloat();

            if (compositeBounds.isEmpty())
                return;

            // YET ANOTHER graphics context to efficiently convert the image to ARGB
            // why? Because later, compositing to the main graphics context (g) is faster
            // (won't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
            // which slows down the main compositing by a factor of 2-3x)
            // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76
            compositedARGB = { juce::Image::ARGB, (int) compositeBounds.getWidth(), (int) compositeBounds.getHeight(), true };

            // we're already scaled up (if needed) so no .addTransform here
            juce::Graphics g2 (compositedARGB);

            for (auto& shadow : renderedSingleChannelShadows)
            {
                auto scaledAndPlaced = juce::AffineTransform::scale (scale);
                auto pathCopy = lastOriginAgnosticPath;
                pathCopy.applyTransform (scaledAndPlaced);

                auto shadowPosition = shadow.getScaledBounds().getPosition();

                // this particular single channel blur might have a different offset from the overall composite
                auto shadowOffsetFromComposite = shadowPosition - compositeBounds.getPosition();

                // lets us temporarily clip the region if needed
                juce::Graphics::ScopedSaveState saveState (g2);

                g2.setColour (shadow.parameters.color);

                // for inner shadows, clip to the path bounds
                // we are doing this here instead of in the single channel render
                // because we want the render to contain the full shadow
                // so it's cheap to move / recolor / etc
                if (shadow.parameters.inner)
                {
                    // we've already saved the state, now clip to the path
                    // this needs to be a path, not bounds!
                    // the goal is to not paint anything outside of these bounds
                    g2.reduceClipRegion (pathCopy);

                    // Inner shadows often have areas which needed to be filled with pure shadow colors
                    // For example, when offsets are greater than radius
                    // This matches figma, css, etc.
                    // Otherwise the shadow will be clipped (and have a hard edge).
                    // Since the shadows are square and at integer pixels,
                    // we fill the edges that lie between our shadow and path bounds

                    // where is our square cached shadow relative to our composite
                    auto shadowBounds = shadow.getScaledBounds();

                    /* In the case the shadow is smaller (due to spread):

                        ptl┌───────────────┐
                           │               │
                           │  stl┌───┐     │
                           │     │   │     │
                           │     └───┘sbr  │
                           │               │
                           └───────────────┘pbr

                     Or the shadow image doesn't fully cover the path (offset > radius)
                          stl┌──────────┐
                             │          │
                       ptl┌──┼──┐       │
                          │  │  │       │
                          │  │  │       │
                          └──┼──┘pbr    │
                             │          │
                             └──────────┘sbr

                     */
                    auto ptl = shadow.getScaledPathBounds().getTopLeft();
                    auto pbr = shadow.getScaledPathBounds().getBottomRight();
                    auto stl = shadowBounds.getTopLeft();
                    auto sbr = shadowBounds.getBottomRight();

                    auto topEdge = juce::Rectangle<int> (ptl.x, ptl.y, pbr.x, stl.y);
                    auto leftEdge = juce::Rectangle<int> (ptl.x, ptl.y, stl.x, pbr.y);
                    auto bottomEdge = juce::Rectangle<int> (ptl.x, sbr.y, pbr.x, pbr.y);
                    auto rightEdge = juce::Rectangle<int> (sbr.x, ptl.y, pbr.x, pbr.y);

                    g2.fillRect (topEdge);
                    g2.fillRect (leftEdge);
                    g2.fillRect (bottomEdge);
                    g2.fillRect (rightEdge);
                }

                // "true" means "fill the alpha channel with the current brush" — aka s.color
                // this is a bit deceptive for the drawImageAt call
                // it will literally g2.fillAll() with the shadow's color
                // using the shadow's image as a sort of mask
                g2.drawImageAt (shadow.getImage(), shadowOffsetFromComposite.getX(), shadowOffsetFromComposite.getY(), true);
            }
            needsRecomposite = false;
        }
    };
}
