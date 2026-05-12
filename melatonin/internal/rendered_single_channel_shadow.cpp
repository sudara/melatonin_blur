#include "rendered_single_channel_shadow.h"
#include "implementations.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin::internal
{
    RenderedSingleChannelShadow::RenderedSingleChannelShadow (ShadowParametersInt p)
        : parameters (p)
   #if MELATONIN_BLUR_USE_DIRECT2D
        , direct2DBlur (std::make_unique<melatonin::blur::Direct2DSingleChannelBlur>())
   #endif
    {}

    juce::Image& RenderedSingleChannelShadow::render (juce::Path& originAgnosticPath, float scale, bool stroked)
    {
        jassert (scale > 0);
        scaledPathBounds = (originAgnosticPath.getBounds() * scale).getSmallestIntegerContainer();
        updateScaledShadowBounds (scale);

        if (scaledShadowBounds.isEmpty())
        {
           #if MELATONIN_BLUR_USE_DIRECT2D
            singleChannelMask = juce::Image();
           #endif
            singleChannelRender = juce::Image();
            return singleChannelRender;
        }

        // We can't modify our original path as it would break cache.
        // Remember, the origin of the path will always be 0,0
        auto shadowPath = juce::Path (originAgnosticPath);

        // radius < 1 falls through on purpose: spread alone still produces a shape, blur becomes a no-op
        if (!stroked && parameters.spread != 0)
        {
            // expand the actual path itself
            // note: this is 1x, it'll be upscaled as needed by fillPath
            auto bounds = originAgnosticPath.getBounds().expanded (parameters.inner ? (float) -parameters.spread : (float) parameters.spread);
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
        }

        // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
        if (parameters.inner)
        {
            shadowPath.setUsingNonZeroWinding (false);

            // The outside of our path will be filled with shadow color
            // which will then cast a blurred shadow inside the path's area.
            // We need a radius amount of pixels for a strong shadow at the path's edge
            shadowPath.addRectangle (shadowPath.getBounds().expanded ((float) scaledRadius));
        }

        const auto targetBounds = scaledShadowBounds.withZeroOrigin();

       #if MELATONIN_BLUR_USE_DIRECT2D
        // Below the pixel threshold D2D setup beats the actual GPU work.
        // Captured once and passed down so a concurrent setDirect2DEnabled()
        // can't desync prepareImagesForRender / blurInto mid-render.
        const auto useDirect2D = melatonin::blur::isDirect2DEnabled()
                                 && targetBounds.getWidth() * targetBounds.getHeight() >= melatonin::blur::direct2DMinimumSingleChannelPixels;
       #else
        constexpr bool useDirect2D = false;
       #endif

        auto& pathTarget = prepareImagesForRender (targetBounds, useDirect2D);

        {
            juce::Graphics g2 (pathTarget);
            forceClearTransparent (g2, targetBounds);

            // ensure we're working at the correct scale
            g2.addTransform (juce::AffineTransform::scale (scale));

            // cache at full opacity (later composited with the correct color/opacity)
            g2.setColour (juce::Colours::white);

            // we're still working @1x until fillPath happens
            // blurContextBounds x/y is negative (relative to path @ 0,0) and we must render in positive space
            // Note that offset isn't used here,
            auto unscaledPosition = -scaledShadowBounds.getPosition().toFloat() / scale;

            g2.fillPath (shadowPath, juce::AffineTransform::translation (unscaledPosition));
        }

        blurInto (singleChannelRender, scaledRadius, useDirect2D);
        return singleChannelRender;
    }

    juce::Image& RenderedSingleChannelShadow::prepareImagesForRender (juce::Rectangle<int> bounds, [[maybe_unused]] bool useDirect2D)
    {
        ensureImage (singleChannelRender, juce::Image::SingleChannel, bounds);

       #if MELATONIN_BLUR_USE_DIRECT2D
        if (useDirect2D)
        {
            ensureImage (singleChannelMask, juce::Image::SingleChannel, bounds);
            return singleChannelMask;
        }
       #endif

        return singleChannelRender;
    }

    void RenderedSingleChannelShadow::blurInto (juce::Image& dst, int radius, [[maybe_unused]] bool useDirect2D)
    {
       #if MELATONIN_BLUR_USE_DIRECT2D
        if (useDirect2D)
        {
            if (direct2DBlur->render (singleChannelMask, dst, (size_t) radius))
                return;

            // D2D bailed (device loss, page allocation failure, etc.). Copy the
            // rasterized mask into dst so the CPU fallback can blur it in place.
            // Same format on both sides, so convertFrom is just a memcpy per row.
            const juce::Image::BitmapData maskData (singleChannelMask, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData renderData (dst, juce::Image::BitmapData::writeOnly);
            renderData.convertFrom (maskData);

            melatonin::blur::cpuSingleChannel (dst, (size_t) radius);
            return;
        }

        // useDirect2D == false: prepareImagesForRender rasterized straight into dst.
        melatonin::blur::cpuSingleChannel (dst, (size_t) radius);
       #else
        // perform the blur with the fastest algorithm available
        melatonin::blur::singleChannel (dst, (size_t) radius);
       #endif
    }

    // Offset is added on the fly, it's not actually a part of the render
    // and can change without invalidating cache
    juce::Rectangle<int> RenderedSingleChannelShadow::getScaledBounds()
    {
        return scaledShadowBounds + scaledOffset;
    }

    juce::Rectangle<int> RenderedSingleChannelShadow::getScaledPathBounds()
    {
        return scaledPathBounds;
    }

    const juce::Image& RenderedSingleChannelShadow::getImage()
    {
        return singleChannelRender;
    }

    bool RenderedSingleChannelShadow::updateRadius (int radius)
    {
        if (juce::approximatelyEqual (radius, parameters.radius))
            return false;

        parameters.radius = radius;
        return true;
    }

    bool RenderedSingleChannelShadow::updateSpread (int spread)
    {
        if (juce::approximatelyEqual (spread, parameters.spread))
            return false;

        parameters.spread = spread;
        return true;
    }

    bool RenderedSingleChannelShadow::updateOffset (juce::Point<int> offset, float scale)
    {
        if (offset == parameters.offset)
            return false;

        parameters.offset = offset;
        scaledOffset = (parameters.offset * scale).roundToInt();
        return true;
    }

    bool RenderedSingleChannelShadow::updateColor (juce::Colour color)
    {
        if (color == parameters.color)
            return false;

        parameters.color = color;
        return true;
    }

    bool RenderedSingleChannelShadow::updateOpacity (float opacity)
    {
        if (juce::approximatelyEqual (opacity, parameters.color.getFloatAlpha()))
            return false;

        parameters.color = parameters.color.withAlpha (opacity);
        return true;
    }

    // this doesn't re-render, just re-calculates position stuff
    void RenderedSingleChannelShadow::updateScaledShadowBounds (float scale)
    {
        // By default, match the main graphics context's scaling factor.
        // This lets us render retina / high quality shadows.
        // We can only use an integer numbers for blurring (hence the rounding)
        scaledSpread = juce::roundToInt ((float) parameters.spread * scale);
        scaledRadius = juce::roundToInt ((float) parameters.radius * scale);
        scaledOffset = (parameters.offset * scale).roundToInt();

        // account for our scaled radius and spread
        // one might think that inner shadows don't need to expand with radius
        // since they are clipped to path bounds
        // however, when there's offset, and we are making position-agnostic shadows!
        if (parameters.inner)
        {
            scaledShadowBounds = scaledPathBounds.expanded (scaledRadius - scaledSpread, scaledRadius - scaledSpread);
        }
        else
            scaledShadowBounds = scaledPathBounds.expanded (scaledRadius + scaledSpread, scaledRadius + scaledSpread);

        // TODO: Investigate/test if this is ever relevant / how to apply to position agnostic
        // I'm guessing reduces the clip size in the edge case it doesn't overlap the main context?
        // It comes from JUCE's shadow classes
        //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

        // if the scale isn't an integer, we'll be dealing with subpixel compositing
        // for example a 4.5px image centered in a canvas technically has the width of 6 pixels
        // (the outer 2 pixels will be 25%-ish opacity)
        // this is a problem because we're going to be blurring the image
        // and don't want to cut our blurs off early
        if (!juce::approximatelyEqual (scale - std::floor (scale), 0.0f))
        {
            // lazily add a buffer all around the image for sub-pixel-ness
            scaledShadowBounds.expand (1, 1);
        }
    }
}
