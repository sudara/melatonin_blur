#include "implementations.h"

namespace melatonin::internal
{

juce::Image& RenderedSingleChannelShadow::render (juce::Path& originAgnosticPath, float scale, bool stroked)
{
    jassert (scale > 0);
    scaledPathBounds = (originAgnosticPath.getBounds() * scale).getSmallestIntegerContainer();
    updateScaledShadowBounds (scale);

    // explicitly support 0 radius shadows and edge spread cases
    if (parameters.radius < 1 || scaledShadowBounds.isEmpty())
        singleChannelRender = juce::Image();

    // We can't modify our original path as it would break cache.
    // Remember, the origin of the path will always be 0,0
    auto shadowPath = juce::Path (originAgnosticPath);

    if (!stroked && parameters.spread != 0)
    {
        // expand the actual path itself
        // note: this is 1x, it'll be upscaled as needed by fillPath
        auto bounds = originAgnosticPath.getBounds().expanded (parameters.inner ? (float) -parameters.spread : (float) parameters.spread);
        shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
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

    // each shadow is its own single channel image associated with a color
    juce::Image renderedSingleChannel (juce::Image::SingleChannel, scaledShadowBounds.getWidth(), scaledShadowBounds.getHeight(), true);

    // boot up a graphics context to give us access to fillPath, etc
    juce::Graphics g2 (renderedSingleChannel);

    // ensure we're working at the correct scale
    g2.addTransform (juce::AffineTransform::scale (scale));

    // cache at full opacity (later composited with the correct color/opacity)
    g2.setColour (juce::Colours::white);

    // we're still working @1x until fillPath happens
    // blurContextBounds x/y is negative (relative to path @ 0,0) and we must render in positive space
    // Note that offset isn't used here,
    auto unscaledPosition = -scaledShadowBounds.getPosition().toFloat() / scale;

    g2.fillPath (shadowPath, juce::AffineTransform::translation (unscaledPosition));

    // perform the blur with the fastest algorithm available
    melatonin::blur::singleChannel (renderedSingleChannel, (size_t) scaledRadius);

    singleChannelRender = renderedSingleChannel;
    return singleChannelRender;
}

}