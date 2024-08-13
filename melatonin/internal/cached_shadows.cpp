#include "cached_shadows.h"

namespace melatonin::internal
{
    CachedShadows::CachedShadows (std::initializer_list<ShadowParameters> shadowParameters, bool force_inner)
    {
        for (auto& parameters : shadowParameters)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (parameters);

            if (force_inner)
                shadow.parameters.inner = true;
        }
    }

    CachedShadows::CachedShadows (const std::vector<ShadowParameters>& shadowParameters, bool force_inner)
    {
        for (auto& parameters : shadowParameters)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (parameters);

            if (force_inner)
                shadow.parameters.inner = true;
        }
    }

    void CachedShadows::render (juce::Graphics& g, const juce::Path& newPath, bool lowQuality)
    {
        TRACE_COMPONENT();

        // On render, there might not be a shadow yet (we can add one later)
        // The path might be empty due to messy resize/paint logic on initialization.
        if (renderedSingleChannelShadows.empty() || newPath.getBounds().isEmpty())
            return;

        setScale (g, lowQuality);

        // Store a copy of the path.
        juce::Path pathCopy (newPath);

        // If it's new to us, strip its location and store its float x/y offset to 0,0
        updatePathIfNeeded (pathCopy);

        renderInternal (g);
    }

    void CachedShadows::render (juce::Graphics& g, const juce::Path& newPath, const juce::PathStrokeType& newType, bool lowQuality)
    {
        TRACE_COMPONENT();

        if (renderedSingleChannelShadows.empty())
            return;

        stroked = true;
        setScale (g, lowQuality);
        if (newType != strokeType)
        {
            strokeType = newType;
            needsRecalculate = true;
        }

        // Stroking the path changes its bounds.
        // Do this before we strip the origin and compare with cache.
        juce::Path strokedPath;
        strokeType.createStrokedPath (strokedPath, newPath, {}, scale);

        updatePathIfNeeded (strokedPath);

        renderInternal (g);
    }

    void CachedShadows::render (juce::Graphics& g, const juce::String& text, const juce::Rectangle<float>& area, juce::Justification justification)
    {
        TRACE_COMPONENT();

        if (renderedSingleChannelShadows.empty())
            return;

        setScale (g, false);

        // TODO: right now if text is repositioned it *will* break blur cache
        // This seems favorable than calling arrangement.createPath on each call?
        // But if you are animating text shadows and grumpy at performance, please open an issue :)
        TextArrangement newTextArrangement { text, g.getCurrentFont(), area, justification };
        if (newTextArrangement != lastTextArrangement)
        {
            lastTextArrangement = newTextArrangement;
            juce::GlyphArrangement arr;
            arr.addLineOfText (g.getCurrentFont(), text, area.getX(), area.getY());
            arr.justifyGlyphs (0, arr.getNumGlyphs(), area.getX(), area.getY(), area.getWidth(), area.getHeight(), justification);
            juce::Path path;
            arr.createPath (path);
            updatePathIfNeeded (path);
        }

        renderInternal (g);
        // need to still render a path here, which path?
    }

    void CachedShadows::render (juce::Graphics& g, const juce::String& text, const juce::Rectangle<int>& area, juce::Justification justification)
    {
        render (g, text, area.toFloat(), justification);
    }

    void CachedShadows::render (juce::Graphics& g, const juce::String& text, int x, int y, int width, int height, juce::Justification justification)
    {
        render (g, text, juce::Rectangle<int> (x, y, width, height).toFloat(), justification);
    }

    CachedShadows& CachedShadows::setRadius (size_t radius, size_t index)
    {
        if (canUpdateShadow (index))
            needsRecalculate = renderedSingleChannelShadows[index].updateRadius ((int) radius);

        return *this;
    }

    CachedShadows& CachedShadows::setSpread (size_t spread, size_t index)
    {
        if (canUpdateShadow (index))
            needsRecalculate = renderedSingleChannelShadows[index].updateSpread ((int) spread);

        return *this;
    }

    CachedShadows& CachedShadows::setOffset (juce::Point<int> offset, size_t index)
    {
        if (canUpdateShadow (index))
            needsRecomposite = renderedSingleChannelShadows[index].updateOffset (offset, scale);

        return *this;
    }

    CachedShadows& CachedShadows::setColor (juce::Colour color, size_t index)
    {
        if (canUpdateShadow (index))
            needsRecomposite = renderedSingleChannelShadows[index].updateColor (color);

        return *this;
    }

    CachedShadows& CachedShadows::setOpacity (float opacity, size_t index)
    {
        if (canUpdateShadow (index))
            needsRecomposite = renderedSingleChannelShadows[index].updateOpacity (opacity);

        return *this;
    }

    bool CachedShadows::TextArrangement::operator== (const TextArrangement& other) const
    {
        return text == other.text && font == other.font && area == other.area && justification == other.justification;
    }

    bool CachedShadows::TextArrangement::operator!= (const TextArrangement& other) const
    {
        return !(*this == other);
    }

    bool CachedShadows::canUpdateShadow (size_t index)
    {
        // this is a nice-to-have
        // it means we can short circuit rendering, but still have a default shadow
        // when we use the setters
        if (index == 0 && renderedSingleChannelShadows.empty())
            renderedSingleChannelShadows.emplace_back (emptyShadow());

        return index < renderedSingleChannelShadows.size();
    }

    void CachedShadows::setScale (juce::Graphics& g, bool lowQuality)
    {
        // Before Melatonin Blur, it was all low quality!
        float newScale = 1.0;
        if (!lowQuality)
            newScale = g.getInternalContext().getPhysicalPixelScaleFactor();

        // break cache if we're painting on a different monitor, etc
        if (!juce::approximatelyEqual (scale, newScale))
        {
            needsRecalculate = true;
            scale = newScale;
        }
    }

    void CachedShadows::updatePathIfNeeded (juce::Path& pathToBlur)
    {
        TRACE_COMPONENT();

        // stripping the origin lets us animate/translate paths in our UI without breaking blur cache
        const auto incomingOrigin = pathToBlur.getBounds().getPosition().toDouble();
        pathToBlur.applyTransform (juce::AffineTransform::translation (-incomingOrigin));

        // has the path actually changed?
        if (needsRecalculate || (!approximatelyEqualPaths (pathToBlur, lastOriginAgnosticPath)))
        {
            // we already created a copy (that is passed in here), this is faster than creating another
            lastOriginAgnosticPath.swapWithPath (pathToBlur);

            // we'll need this later for compositing
            // TODO: Do we really need to store two copies?
            lastOriginAgnosticPathScaled = lastOriginAgnosticPath;
            lastOriginAgnosticPathScaled.applyTransform (juce::AffineTransform::scale (scale));

            // remember the new placement in the context
            pathPositionInContext = incomingOrigin;

            needsRecalculate = true;
        }
        else if (!juce::approximatelyEqual (incomingOrigin, pathPositionInContext))
        {
            // reposition the cached single channel shadows
            pathPositionInContext = incomingOrigin;
        }
    }

    void CachedShadows::recalculateBlurs()
    {
        TRACE_COMPONENT();

        for (auto& shadow : renderedSingleChannelShadows)
        {
            shadow.render (lastOriginAgnosticPath, scale, stroked);
        }
        needsRecalculate = false;
        needsRecomposite = true;
    }

    void CachedShadows::renderInternal (juce::Graphics& g)
    {
        TRACE_COMPONENT();

        // if it's a new path or the path actually changed, redo the single channel blurs
        if (needsRecalculate)
            recalculateBlurs();

        // have any of the shadows changed position/color/opacity OR been recalculated?
        // if so, recreate the ARGB composite of all the shadows together
        if (needsRecomposite)
            compositeShadowsToARGB();

        // draw the cached composite into the main graphics context
        drawARGBComposite (g);
    }

    void CachedShadows::drawARGBComposite (juce::Graphics& g, bool optimizeClipBounds)
    {
        TRACE_COMPONENT();

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

        // compositedARGB has been scaled by the physical pixel scale factor
        // (unless lowQuality is true)
        // we have to pass a 1/scale transform because the context will otherwise try to scale the image up
        // (which is not what we want, at this point our cached shadow is 1:1 with the context)
        auto position = scaledCompositePosition + (pathPositionInContext * scale);
        g.drawImageTransformed (compositedARGB, juce::AffineTransform::translation (position).scaled (1.0f / scale));
    }

    void CachedShadows::compositeShadowsToARGB()
    {
        TRACE_COMPONENT();

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

        scaledCompositePosition = compositeBounds.getPosition().toDouble();

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
            // TODO: no reason for this scaled copy to be in the loop
            auto pathCopy = lastOriginAgnosticPathScaled;

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
                // TODO: This fails for stroked paths!
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

    // Debug: 20-80µs
    // Release: 2-4µs
    // Unfortunately, path comparison in JUCE doesn't hold up to translation and float inaccuracies
    // see: https://forum.juce.com/t/should-juce-path-equality-use-approximatelyequal/59739/2
    bool CachedShadows::approximatelyEqualPaths (const juce::Path& first, const juce::Path& second, float tolerance)
    {
        TRACE_COMPONENT();

        if (first.isEmpty() && second.isEmpty())
            return true;

        const auto t = juce::Tolerance<float> {}.withAbsolute (tolerance);

        // first check bounds
        const auto firstBounds = first.getBounds();
        const auto secondBounds = second.getBounds();
        if (!juce::approximatelyEqual (firstBounds.getX(), secondBounds.getX(), t)
            || !juce::approximatelyEqual (firstBounds.getY(), secondBounds.getY(), t)
            || !juce::approximatelyEqual (firstBounds.getWidth(), secondBounds.getWidth(), t)
            || !juce::approximatelyEqual (firstBounds.getHeight(), secondBounds.getHeight(), t))
            return false;


        // Next, check a few random points in the path
        // sample a couple random points and make sure things are identical
        for (auto i = 0; i < 2; ++i)
        {
            const auto testX = firstBounds.getX() + juce::Random::getSystemRandom().nextFloat() * firstBounds.getWidth();
            const auto testY = firstBounds.getY() + juce::Random::getSystemRandom().nextFloat() * firstBounds.getHeight();
            const juce::Point randomPoint = { testX, testY };
            juce::Point<float> firstFoundPoint;
            juce::Point<float> secondFoundPoint;
            first.getNearestPoint (randomPoint, firstFoundPoint);
            second.getNearestPoint (randomPoint, secondFoundPoint);
            if (!juce::approximatelyEqual (firstFoundPoint.getX(), secondFoundPoint.getX(), t)
                || !juce::approximatelyEqual (firstFoundPoint.getY(), secondFoundPoint.getY(), t))
                return false;
        }

        return true;
    }
}
