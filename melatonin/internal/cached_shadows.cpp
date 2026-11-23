#include "cached_shadows.h"
#include "implementations.h"

namespace melatonin::internal
{
    CachedShadows::CachedShadows (std::initializer_list<ShadowParametersInt> shadowParameters, const bool force_inner)
    {
        for (auto& parameters : shadowParameters)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (parameters);

            if (force_inner)
                shadow.parameters.inner = true;
        }
    }

    CachedShadows::CachedShadows (const std::vector<ShadowParametersInt>& shadowParameters, const bool force_inner)
    {
        for (auto& parameters : shadowParameters)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (parameters);

            if (force_inner)
                shadow.parameters.inner = true;
        }
    }

    CachedShadows::CachedShadows (const std::initializer_list<int> radii, const bool isInner)
    {
        for (const auto radius : radii)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (ShadowParametersInt { juce::Colours::black, radius });
            if (isInner)
                shadow.parameters.inner = true;
        }
    }

    CachedShadows::CachedShadows (const std::initializer_list<double> radii, const bool isInner)
    {
        for (const auto radius : radii)
        {
            auto& shadow = renderedSingleChannelShadows.emplace_back (ShadowParametersInt { juce::Colours::black, juce::roundToInt (radius) });
            if (isInner)
                shadow.parameters.inner = true;
        }
    }

    void CachedShadows::render (juce::Graphics& g, const juce::Path& newPath, const bool lowQuality)
    {
        // on render, there might not be a shadow yet (we can add one later)
        // and the path might be empty (usually due to messy resize/paint logic)
        if (renderedSingleChannelShadows.empty() || newPath.getBounds().isEmpty())
            return;

        pathIsFilled = true;
        setScale (g, lowQuality);

        // Store a copy of the path.
        juce::Path pathCopy (newPath);

        // If it's new to us, strip its location and store its float x/y offset to 0,0
        updatePathIfNeeded (pathCopy);

        renderInternal (g);
    }

    void CachedShadows::render (juce::Graphics& g, const juce::Path& newPath, const juce::PathStrokeType& newType, const bool lowQuality, const juce::AffineTransform& transform)
    {
        if (renderedSingleChannelShadows.empty())
            return;

        stroked = true;
        pathIsFilled = false;
        setScale (g, lowQuality);
        if (newType != strokeType)
        {
            strokeType = newType;
            needsRecalculate = true;
        }

        // Stroking the path changes its bounds.
        // Do this before we strip the origin and compare with cache.
        juce::Path strokedPath;
        strokeType.createStrokedPath (strokedPath, newPath, transform, scale);

        updatePathIfNeeded (strokedPath);

        renderInternal (g);
    }

    void CachedShadows::render (juce::Graphics& g, const juce::String& text, const juce::Rectangle<float>& area, juce::Justification justification)
    {
        if (renderedSingleChannelShadows.empty())
            return;

        pathIsFilled = false;
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

    CachedShadows& CachedShadows::setRadius (const double radius, const size_t index)
    {
        if (canUpdateShadow (index))
            needsRecalculate |= renderedSingleChannelShadows[index].updateRadius ((int) radius);

        return *this;
    }

    CachedShadows& CachedShadows::setSpread (const double spread, const size_t index)
    {
        if (canUpdateShadow (index))
            needsRecalculate |= renderedSingleChannelShadows[index].updateSpread ((int) spread);

        return *this;
    }

    CachedShadows& CachedShadows::setOffset (const juce::Point<int> offset, const size_t index)
    {
        if (canUpdateShadow (index))
            needsRecomposite |= renderedSingleChannelShadows[index].updateOffset (offset, scale);

        return *this;
    }

    CachedShadows& CachedShadows::setColor (juce::Colour color, size_t index)
    {
        if (canUpdateShadow (index))
        {
            auto& shadow = renderedSingleChannelShadows[index];
            const auto previous = shadow.parameters.color;

            if (shadow.updateColor (color))
                needsRecomposite |= ! isSingleDropShadow() || previous.withAlpha (1.0f) != color.withAlpha (1.0f);
        }

        return *this;
    }

    CachedShadows& CachedShadows::setOpacity (double opacity, size_t index)
    {
        if (canUpdateShadow (index) && renderedSingleChannelShadows[index].updateOpacity (static_cast<float> (opacity)))
            needsRecomposite |= ! isSingleDropShadow();

        return *this;
    }

    CachedShadows& CachedShadows::excludeInterior (const float inset, const bool shouldExclude)
    {
        jassert (inset >= 0.0f);
        interiorInset = shouldExclude ? inset : -1.0f;
        return *this;
    }

    bool CachedShadows::isSingleDropShadow() const
    {
        return renderedSingleChannelShadows.size() == 1 && ! renderedSingleChannelShadows.front().parameters.inner;
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
        // stripping the origin lets us animate/translate paths in our UI without breaking blur cache
        auto incomingOrigin = pathToBlur.getBounds().getPosition();
        pathToBlur.applyTransform (juce::AffineTransform::translation (-incomingOrigin));

        // has the path actually changed?
        if (needsRecalculate || (pathToBlur != lastOriginAgnosticPath))
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
        else if (incomingOrigin != pathPositionInContext)
        {
            // reposition the cached single channel shadows
            pathPositionInContext = incomingOrigin;
        }
    }

    void CachedShadows::recalculateBlurs()
    {
        for (auto& shadow : renderedSingleChannelShadows)
        {
            shadow.render (lastOriginAgnosticPath, scale, stroked);
        }

       #if MELATONIN_BLUR_BLEND_COMPOSITE
        renderPathCoverage();
       #endif

        needsRecalculate = false;
        needsRecomposite = true;
    }

    void CachedShadows::renderInternal (juce::Graphics& g)
    {
        // If the D2D backend has been toggled at runtime since we last rendered,
        // our cached single-channel images came from a different code path and
        // must be redone. Cheap: one relaxed atomic load + integer compare.
        if (const auto gen = melatonin::blur::direct2DConfigGeneration(); gen != lastDirect2DGeneration)
        {
            needsRecalculate = true;
            lastDirect2DGeneration = gen;
        }

        // "Uncached" debug mode: force a full recalc each render. Useful for
        // perf measurement and for tools that want to observe the worst-case
        // (cold-cache) cost of every frame.
        if (bypassCache)
            needsRecalculate = true;

        // if it's a new path or the path actually changed, redo the single channel blurs
        if (needsRecalculate)
            recalculateBlurs();

        float drawOpacity = 1.0f;
        if (isSingleDropShadow())
        {
            const auto color = renderedSingleChannelShadows.front().parameters.color;
            // A changing alpha is applied at draw time, so a fade does not rebuild the image on each frame. A steady alpha is baked in, which draws faster.
            const bool fading = color.getAlpha() != lastDrawnAlpha && compositedARGB.isValid();
            const auto alphaToCache = fading ? juce::uint8 { 255 } : color.getAlpha();
            needsRecomposite |= alphaToCache != compositeAlpha;
            compositeAlpha = alphaToCache;
            lastDrawnAlpha = color.getAlpha();
            if (fading)
                drawOpacity = color.getFloatAlpha();
        }

        // have any of the shadows changed position/color/opacity OR been recalculated?
        // if so, recreate the ARGB composite of all the shadows together
        if (needsRecomposite)
            compositeShadowsToARGB();

        // draw the cached composite into the main graphics context
        drawARGBComposite (g, drawOpacity);
    }

    juce::Rectangle<int> CachedShadows::interiorToSkip() const
    {
        if (interiorInset < 0.0f || ! pathIsFilled)
            return {};

        // The shadow can show through antialiased edge pixels even on a square corner.
        const auto solidInterior = lastOriginAgnosticPath.getBounds().reduced (juce::jmax (1.0f, interiorInset));
        auto interior = solidInterior;
        size_t innerShadows = 0;

        for (const auto& shadow : renderedSingleChannelShadows)
        {
            if (! shadow.parameters.inner)
                continue;

            ++innerShadows;

            // Radius, spread and offset each round independently in device pixels.
            const auto reach = (float) (shadow.parameters.radius + juce::jmax (0, shadow.parameters.spread)) + 2.0f / juce::jmin (1.0f, scale);
            interior = interior.getIntersection (solidInterior.reduced (reach + (float) std::abs (shadow.parameters.offset.x), reach + (float) std::abs (shadow.parameters.offset.y)));
        }

        // The drop shadow can still be visible where the inner shadow is transparent.
        if (innerShadows != 0 && innerShadows != renderedSingleChannelShadows.size())
            return {};

        interior += pathPositionInContext;

        // Inward rounding keeps the clip inside the covered area.
        const auto hole = juce::Rectangle<int>::leftTopRightBottom ((int) std::ceil (interior.getX()), (int) std::ceil (interior.getY()), (int) std::floor (interior.getRight()), (int) std::floor (interior.getBottom()));

        if (hole.isEmpty())
            return {};

        // Setting up the clip can cost more than drawing a small interior.
        const auto devicePixels = (float) hole.getWidth() * (float) hole.getHeight() * scale * scale;
        return devicePixels >= 150000.0f ? hole : juce::Rectangle<int>();
    }

    void CachedShadows::drawARGBComposite (juce::Graphics& g, float opacity)
    {
        // support default constructors, 0 radius blurs, etc
        if (compositedARGB.isNull())
            return;

        // resets the Clip Region when this scope ends
        juce::Graphics::ScopedSaveState saveState (g);

        // Direct2D clips to a hole with a geometry layer, which is slow.
       #if ! (JUCE_WINDOWS && JUCE_MAJOR_VERSION >= 8)
        // A rectangular exclusion avoids antialiasing a second path around the interior.
        if (const auto hole = interiorToSkip(); ! hole.isEmpty())
            g.excludeClipRegion (hole);
       #endif

        g.setOpacity (opacity);

        // compositedARGB has been scaled by the physical pixel scale factor
        // (unless lowQuality is true)
        // we have to pass a 1/scale transform because the context will otherwise try to scale the image up
        // (which is not what we want, at this point our cached shadow is 1:1 with the context)
        auto position = scaledCompositePosition + (pathPositionInContext * scale);
        g.drawImageTransformed (compositedARGB, juce::AffineTransform::translation (position).scaled (1.0f / scale));
    }

    void CachedShadows::compositeShadowsToARGB()
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
        const auto targetBounds = compositeBounds.withZeroOrigin();
        ensureImage (compositedARGB, juce::Image::ARGB, targetBounds);

       #if MELATONIN_BLUR_BLEND_COMPOSITE
        blendShadowsIntoComposite (compositeBounds);
       #else
        // we're already scaled up (if needed) so no .addTransform here
        juce::Graphics g2 (compositedARGB);
        forceClearTransparent (g2, targetBounds);

        for (auto& shadow : renderedSingleChannelShadows)
        {
            // TODO: no reason for this scaled copy to be in the loop
            auto pathCopy = lastOriginAgnosticPathScaled;

            auto shadowPosition = shadow.getScaledBounds().getPosition();

            // this particular single channel blur might have a different offset from the overall composite
            auto shadowOffsetFromComposite = shadowPosition - compositeBounds.getPosition();

            // lets us temporarily clip the region if needed
            juce::Graphics::ScopedSaveState saveState (g2);

            g2.setColour (isSingleDropShadow() ? shadow.parameters.color.withAlpha (compositeAlpha) : shadow.parameters.color);

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
       #endif
        needsRecomposite = false;
    }

   #if MELATONIN_BLUR_BLEND_COMPOSITE
    // A lookup avoids premultiplying the same colour for each pixel in the layer.
    static std::array<juce::uint32, 256> pixelsByCoverage (juce::Colour colour)
    {
        std::array<juce::uint32, 256> pixels;

        for (size_t coverage = 0; coverage < pixels.size(); ++coverage)
        {
            const auto alpha = colour.getFloatAlpha() * (float) coverage;
            pixels[coverage] = juce::PixelARGB ((juce::uint8) juce::roundToInt (alpha),
                (juce::uint8) juce::roundToInt (alpha * colour.getFloatRed()),
                (juce::uint8) juce::roundToInt (alpha * colour.getFloatGreen()),
                (juce::uint8) juce::roundToInt (alpha * colour.getFloatBlue())).getNativeARGB();
        }

        return pixels;
    }

    static juce::uint32 blendOver (const juce::uint32 destination, const juce::uint32 source)
    {
        const auto keep = 255u - (source >> 24);
        // Each channel has a spare byte for the multiply and rounding before division by 255.
        const auto kept = [keep] (juce::uint32 twoChannels) {
            const auto product = twoChannels * keep + 0x00800080u;
            return ((product + ((product >> 8) & 0x00ff00ffu)) >> 8) & 0x00ff00ffu;
        };

        return source + (kept (destination & 0x00ff00ffu) | (kept ((destination >> 8) & 0x00ff00ffu) << 8));
    }

    void CachedShadows::renderPathCoverage()
    {
        const auto isInner = [] (const RenderedSingleChannelShadow& shadow) { return shadow.parameters.inner; };
        const auto bounds = renderedSingleChannelShadows.front().getScaledPathBounds();

        if (bounds.isEmpty() || std::none_of (renderedSingleChannelShadows.begin(), renderedSingleChannelShadows.end(), isInner))
        {
            pathCoverage = {};
            return;
        }

        ensureImage (pathCoverage, juce::Image::SingleChannel, bounds.withZeroOrigin());

        juce::Graphics g (pathCoverage);
        forceClearTransparent (g, pathCoverage.getBounds());
        g.setColour (juce::Colours::white);
        g.fillPath (lastOriginAgnosticPathScaled, juce::AffineTransform::translation (-bounds.getPosition().toFloat()));
    }

    void CachedShadows::blendShadowsIntoComposite (const juce::Rectangle<int> compositeBounds)
    {
        juce::Image::BitmapData composite (compositedARGB, juce::Image::BitmapData::writeOnly);
        const juce::Rectangle<int> compositeArea (composite.width, composite.height);

        auto& first = renderedSingleChannelShadows.front();
        const bool firstLayerCoversComposite = ! first.parameters.inner && first.getImage().isValid() && first.getScaledBounds() == compositeBounds;

        if (! firstLayerCoversComposite)
            for (int y = 0; y < composite.height; ++y)
                juce::zeromem (composite.getLinePointer (y), (size_t) composite.width * sizeof (juce::PixelARGB));

        const auto destinationRow = [&] (int x, int y) { return reinterpret_cast<juce::uint32*> (composite.getPixelPointer (x, y)); };

        for (auto& shadow : renderedSingleChannelShadows)
        {
            const auto pixels = pixelsByCoverage (isSingleDropShadow() ? shadow.parameters.color.withAlpha (compositeAlpha) : shadow.parameters.color);
            const auto& blur = shadow.getImage();
            const auto blurArea = blur.getBounds() + (shadow.getScaledBounds().getPosition() - compositeBounds.getPosition());

            std::optional<juce::Image::BitmapData> blurData;
            if (blur.isValid())
                blurData.emplace (blur, juce::Image::BitmapData::readOnly);

            if (! shadow.parameters.inner)
            {
                if (! blurData)
                    continue;

                const auto area = blurArea.getIntersection (compositeArea);
                const bool ontoEmptyComposite = &shadow == &renderedSingleChannelShadows.front();

                for (int y = area.getY(); y < area.getBottom(); ++y)
                {
                    const auto* strength = blurData->getPixelPointer (area.getX() - blurArea.getX(), y - blurArea.getY());
                    auto* destination = destinationRow (area.getX(), y);

                    if (ontoEmptyComposite)
                        for (int x = 0; x < area.getWidth(); ++x)
                            destination[x] = pixels[strength[x]];
                    else
                        for (int x = 0; x < area.getWidth(); ++x)
                            destination[x] = blendOver (destination[x], pixels[strength[x]]);
                }

                continue;
            }

            if (pathCoverage.isNull())
                continue;

            const juce::Image::BitmapData clip (pathCoverage, juce::Image::BitmapData::readOnly);
            const auto pathArea = pathCoverage.getBounds() + (shadow.getScaledPathBounds().getPosition() - compositeBounds.getPosition());
            const auto area = pathArea.getIntersection (compositeArea);

            for (int y = area.getY(); y < area.getBottom(); ++y)
            {
                const auto* coverage = clip.getPixelPointer (area.getX() - pathArea.getX(), y - pathArea.getY());
                const auto* blurRow = blurData && blurArea.getVerticalRange().contains (y) ? blurData->getLinePointer (y - blurArea.getY()) : nullptr;
                auto* destination = destinationRow (area.getX(), y);

                for (int x = 0; x < area.getWidth(); ++x)
                {
                    // An offset can expose the solid part of the inner shadow beyond the stored blur.
                    const auto blurX = area.getX() + x - blurArea.getX();
                    const auto strength = blurRow != nullptr && blurX >= 0 && blurX < blurArea.getWidth() ? blurRow[blurX] : 255u;
                    destination[x] = blendOver (destination[x], pixels[(strength * coverage[x] + 127u) / 255u]);
                }
            }
        }
    }
   #endif
}
