#pragma once
#include "support/helpers.h"

namespace melatonin
{
    class CachedBlur
    {
    public:
        explicit CachedBlur (size_t r)
            : radius (r)
        {
            jassert (radius > 0);
        }

        // we are passing the source by value here
        // (but it's a value object of sorts since its reference counted)
        void update (juce::Image newSource)
        {
            if (newSource != src)
            {
                jassert (newSource.isValid());
                src = newSource;

                // the first time the blur is created, a copy is needed
                // so we are passing correct dimensions, etc to the blur algo
                dst = src.createCopy();
                melatonin::blur::argb (src, dst, radius);
            }
        }

        juce::Image& render (juce::Image& newSource)
        {
            update (newSource);
            return dst;
        }

        juce::Image& render()
        {
            // You either need to have called update or rendered with a src!
            jassert (dst.isValid());
            return dst;
        }

    private:
        // juce::Images are value objects, reference counted behind the scenes
        // We want to store a reference to the src so we can compare on render
        // And we actually are the owner of the dst
        juce::Image src = juce::Image();
        juce::Image dst = juce::Image();
        size_t radius;
    };
}
