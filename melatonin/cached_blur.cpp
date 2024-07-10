#include "cached_blur.h"

namespace melatonin
{

    CachedBlur::CachedBlur (size_t r) : radius (r)
    {
        jassert (radius > 0);
    }

    void CachedBlur::update (const juce::Image& newSource)
    {
        jassert (newSource.isValid());
        src = newSource;

        // the first time the blur is created, a copy is needed
        // so we are passing correct dimensions, etc to the blur algo
        dst = src.createCopy();
        blur::argb (src, dst, radius);
    }

    juce::Image& CachedBlur::render (const juce::Image& newSource)
    {
        // TODO: This doesn't check if image contents have changed
        // Only that a new image is being passed in.
        // This is problematic for juce::ImageEffectFilter::applyEffect
        // because the image passed in will always be the same image.
        // In that case, you can directly call update
        if (needsRedraw || newSource != src)
            update (newSource);

        return dst;
    }

    void CachedBlur::setRadius (size_t newRadius)
    {
        radius = newRadius;
        needsRedraw = true;
    }


    juce::Image& CachedBlur::render()
    {
        // You either need to have called update or rendered with a src!
        jassert (dst.isValid());
        return dst;
    }

}
