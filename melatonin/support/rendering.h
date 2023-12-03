#pragma once

#include "implementations.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <melatonin_blur/tests/helpers/pixel_helpers.h>

namespace melatonin
{
    // these are the parameters required to represent a single drop or inner shadow
    // wish I could put these in shadows.h to help people
    struct ShadowParameters
    {
        // one single color per shadow
        const juce::Colour color = {};
        const int radius = 1;
        const juce::Point<int> offset = { 0, 0 };

        // Spread literally just expands or contracts the *path* size
        // Inverted for inner shadows
        const int spread = 0;

        // an inner shadow is just a modified drop shadow
        bool inner = false;

        // Internal
        // stored here because it's needed to re-composite from cached images
        // each shadow takes up a different amount of space depending on its radius, spread, etc
        // so its placement in the cached image is always unique
        // we need to know where to place the cached image back into the main context
        juce::Rectangle<int> blurContextBoundsScaled = {};
    };

    namespace internal
    {

        // WARNING: Internal only, don't use unless you know what you are doing
        // this caches expensive shadow creation into a Single Channel juce::Image for later compositing
        // Scale is piped in via CachedShadows::getPhysicalPixelScaleFactor
        [[nodiscard]] static inline juce::Image renderShadowToSingleChannel (ShadowParameters& s, const juce::Path& originAgnosticPath, float scale)
        {
            // By default, match the main graphics context's scaling factor.
            // This lets us render retina / high quality shadows.
            // We can only use an integer numbers for blurring (hence the rounding)
            int scaledRadius = juce::roundToInt ((float) s.radius * scale);
            int scaledSpread = juce::roundToInt ((float) s.spread * scale);
            auto scaledOffset = (s.offset * scale).roundToInt();

            // start calculating our cached "canvas" for the path
            // this is the one piece of info we need for later compositing
            // work at the same scale as the graphics context we'll composite to
            // we need to work with integers when creating graphic contexts
            // and we want our path to start at 0, 0
            s.blurContextBoundsScaled = (originAgnosticPath.getBounds() * scale).getSmallestIntegerContainer();

            // account for our scaled radius, spread, offsets
            if (s.inner)
                // we need an extra pixel to fill for casting shadows into the path
                // radius travels *inwards* for inner shadows
                // and spread can only reduce the size of the bounds, which we'll ignore
                s.blurContextBoundsScaled.expand (1, 1);
            else
                s.blurContextBoundsScaled = s.blurContextBoundsScaled.expanded (scaledRadius + scaledSpread);

            // TODO: Investigate/test if this is ever relevant
            // I'm guessing reduces the clip size in the edge case it doesn't overlap the main context?
            //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

            // if the scale isn't an integer, we'll be dealing with subpixel compositing
            // for example a 4.5px image centered in a canvas technically has the width of 6 pixels
            // (the outer 2 pixels will be 25%-ish opacity)
            // this is a problem because we're going to be blurring the image
            // and don't want to cut our blurs off early
            if (!juce::approximatelyEqual (scale - std::floor (scale), 0.0f))
            {
                // lazily add a buffer all around the image for sub-pixel-ness
                s.blurContextBoundsScaled.expand (1, 1);


            }
            // offsets don't add anything to the size of the blur
            // they simply translate placement in the final compositing
            s.blurContextBoundsScaled += scaledOffset;

            // spread modifies the scale of the actual path
            // we can't modify our original path (it would break cache)
            // remember, the origin will always stay at 0,0
            auto shadowPath = juce::Path (originAgnosticPath);

            if (s.spread != 0)
            {
                // expand the actual path itself
                // note: this is 1x, it'll be upscaled as needed by fillPath
                auto bounds = originAgnosticPath.getBounds().expanded (s.inner ? (float) -s.spread : (float) s.spread);
                shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
            }

            // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
            if (s.inner)
            {
                shadowPath.setUsingNonZeroWinding (false);

                // add a single pixel of extra padding
                // since the outside will be filled, this lets us
                // reliably cast a blurred shadow into the path's area
                // TODO: test if edge bleed lets us happily cheat here or if this should be 'radius'
                // TODO: This has impact on figma/css compatibility
                shadowPath.addRectangle (shadowPath.getBounds().expanded (1));
            }

            // Check your parameters: the blur image ended up with a dimension of 0
            // Did you set a negative spread? Check that your path still exists after applying the spread.
            // For example, you can't have a 3x3px path with a -2px spread
            if (s.blurContextBoundsScaled.isEmpty())
            {
                jassertfalse;
                return {};
            }

            // each shadow is its own single channel image associated with a color
            juce::Image renderedSingleChannel (juce::Image::SingleChannel, s.blurContextBoundsScaled.getWidth(), s.blurContextBoundsScaled.getHeight(), true);

            // boot up a graphics context to give us access to fillPath, etc
            juce::Graphics g2 (renderedSingleChannel);

            // ensure we're working at the correct scale
            g2.addTransform (juce::AffineTransform::scale (scale));

            // cache at full opacity, later composited with the correct color/opacity
            g2.setColour (juce::Colours::white);

            // we're still working @1x until fillPath happens
            // blurContextBounds x/y is negative (linked to path @ 0,0) and we must render in positive space
            // blurContextBounds includes offset for later compositing, but we're offset-agnostic for the single render
            // TODO: sudara doesn't understand why scaledOffset is *positive* here
            auto unscaledPosition = (scaledOffset.toFloat() - s.blurContextBoundsScaled.getPosition().toFloat()) / scale;
            g2.fillPath (shadowPath, juce::AffineTransform::translation (unscaledPosition));

            // perform the blur with the fastest algorithm available
            melatonin::blur::singleChannel (renderedSingleChannel, scaledRadius);

            save_test_image (renderedSingleChannel, "singleChan");
            return renderedSingleChannel;
        }
    }
}
