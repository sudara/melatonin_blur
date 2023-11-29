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

        // by default we match the main graphics context's scaling factor
        bool lowQuality = false;

        // an inner shadow is just a modified drop shadow
        bool inner = false;

        // Internal: each shadow takes up a different amount of space depending on its radius, spread, etc
        juce::Rectangle<int> area = {};
    };

    // this caches expensive shadow creation into a ARGB juce::Image for fast compositing
    // WARNING: Internal only, don't use unless you know what you are doing
    // Scale is piped in from CachedShadows::getPhysicalPixelScaleFactor
    [[nodiscard]] static inline juce::Image renderShadowToARGB (ShadowParameters& s, const juce::Path& originalPath, float scale)
    {
        if (s.lowQuality)
            scale = 1.0f;

        // by default, match the main graphics context's scaling factor (render high quality)
        int scaledRadius = juce::roundToInt ((float) s.radius * scale);
        int scaledSpread = juce::roundToInt ((float) s.spread * scale);
        auto scaledOffset = (s.offset * scale).roundToInt();

        // the area of each cached blur depends on its radius and spread
        // 0,0 remains fixed the same thanks to .expanded (it goes negative)
        // we also have to scale s.area itself, because it's later used for compositing
        // so it must reflect the scale of the ARGB image we're returning
        s.area = ((originalPath.getBounds() * scale).getSmallestIntegerContainer() + scaledOffset)
                     .expanded (scaledRadius + scaledSpread);

        // TODO: Investigate/test if this is ever relevant
        // I'm guessing reduces the clip size in the edge case
        // where it wouldn't overlap the main context?
        //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

        // we don't want to modify our original path (it would break cache)
        // additionally, inner shadows render a modified path
        auto shadowPath = juce::Path (originalPath);

        if (scaledSpread != 0)
        {
            // expand the actual path itself
            // note: this is 1x, since it'll be upscaled as needed by fillPath
            auto bounds = originalPath.getBounds().expanded (s.inner ? (float) -s.spread : (float) s.spread);
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
        if (s.inner)
        {
            shadowPath.setUsingNonZeroWinding (false);

            // add an arbitrary amount of extra padding
            // since the outside will be filled, this lets us
            // reliably cast a blurred shadow into the path's area
            // TODO: test if edge bleed lets us happily cheat here or if this should be 'radius'
            shadowPath.addRectangle (shadowPath.getBounds().expanded (2));
        }

        // Check your parameters: the blur image ended up with a dimension of 0
        // Did you set a negative spread? Check that your path still exists after applying the spread.
        // For example, you can't have a 3x3px path with a -2px spread
        if (s.area.isEmpty())
        {
            jassertfalse;
            return {};
        }

        // each shadow is its own single channel image associated with a color
        juce::Image renderedSingleChannel (juce::Image::SingleChannel, s.area.getWidth(), s.area.getHeight(), true);

        // boot up another graphics context to give us access to fillPath, etc
        {
            juce::Graphics g2 (renderedSingleChannel);

            // ensure we're working at the correct scale
            g2.addTransform (juce::AffineTransform::scale (scale));

            g2.setColour (juce::Colours::white);
            g2.fillPath (shadowPath, juce::AffineTransform::translation ((float) (s.offset.x - s.area.getX() / scale), (float) (s.offset.y - s.area.getY() / scale)));
        }

        // perform the blur with the fastest algorithm available
        melatonin::blur::singleChannel (renderedSingleChannel, scaledRadius);

        // YET ANOTHER graphics context to efficiently convert the image to ARGB
        // why? Because later, compositing to the main graphics context becomes (g) faster
        // (don't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
        // which slows down the main compositing by a factor of 2-3x)
        // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76
        juce::Image renderedARGB (juce::Image::ARGB, s.area.getWidth(), s.area.getHeight(), true);
        {
            // note that there's no transform for this context
            // we already scaled up (if needed) the last round
            juce::Graphics g2 (renderedARGB);

            g2.setColour (s.color);
            g2.drawImageAt (renderedSingleChannel, 0, 0, true);
        }
        return renderedARGB;
    }
}
