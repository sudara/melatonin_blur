#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "implementations.h"

namespace melatonin
{
    // these are the parameters required to represent a single drop or inner shadow
    // wish i could put these in shadows.h to help people
    struct ShadowParameters
    {
        // one single color per shadow
        const juce::Colour color = {};
        const int radius = 1;
        const juce::Point<int> offset = { 0, 0 };

        // Spread literally just expands or contracts the path size
        // Inverted for inner shadows
        const int spread = 0;

        // an inner shadow is just a modified drop shadow
        bool inner = false;

        // each shadow takes up a different amount of space depending on it's radius, spread, etc
        juce::Rectangle<int> area = {};
    };

    // this caches the expensive shadow creation into a ARGB juce::Image for fast compositing
    static inline juce::Image renderShadowToARGB (ShadowParameters& s, juce::Path& originalPath)
    {
        // the area of each cached blur depends on its radius and spread
        s.area = (originalPath.getBounds().getSmallestIntegerContainer() + s.offset)
            .expanded (s.radius + s.spread + 1);

        // TODO: Investigate/test what this line does — makes the clip smaller for certain cases?
        //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

        // Reconsider your parameters: one of the dimensions is 0 so the blur doesn't exist!
        if (s.area.getWidth() < 1 || s.area.getHeight() < 1)
            jassertfalse;

        // we don't want to modify our original path (it would break cache)
        // additionally, inner shadows must render a modified path
        auto shadowPath = juce::Path (originalPath);

        if (s.spread != 0)
        {
            // TODO: drop shadow tests for s.area to understand why this is still needed (we expanded above!)
            s.area.expand (s.spread, s.spread);
            auto bounds = originalPath.getBounds().expanded (s.inner ? (float) -s.spread : (float) s.spread);
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
        if (s.inner)
        {
            shadowPath.setUsingNonZeroWinding (false);
            shadowPath.addRectangle (s.area.expanded (10));
        }

        // each shadow is its own single channel image associated with a color
        juce::Image renderedSingleChannel (juce::Image::SingleChannel, s.area.getWidth(), s.area.getHeight(), true);

        // boot up another graphics context to give us access to fillPath, etc
        {
            juce::Graphics g2 (renderedSingleChannel);

            g2.setColour (juce::Colours::white);
            g2.fillPath (shadowPath, juce::AffineTransform::translation ((float) (s.offset.x - s.area.getX()), (float) (s.offset.y - s.area.getY())));
        }

        // perform the blur with the fastest algorithm available
        melatonin::blur::singleChannel (renderedSingleChannel, s.radius);

        // YET ANOTHER graphics context to efficiently convert the image to ARGB
        // why? Because later, compositing to the main graphics context becomes (g) faster
        // (don't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
        // which slows down the main compositing by a factor of 2-3x)
        // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76
        juce::Image renderedARGB (juce::Image::ARGB, s.area.getWidth(), s.area.getHeight(), true);
        {
            juce::Graphics g2 (renderedARGB);
            g2.setColour (s.color);
            g2.drawImageAt (renderedSingleChannel, 0, 0, true);
        }
        return renderedARGB;
    }
}
