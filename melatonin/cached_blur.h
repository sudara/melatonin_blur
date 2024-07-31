#pragma once

namespace melatonin
{
    class CachedBlur
    {
    public:
        explicit CachedBlur (size_t r);

        // we are passing the source by reference here
        // (but it's a value object of sorts since its reference counted)
        void update (const juce::Image& newSource);

        // Render and potentially update the image
        juce::Image& render (const juce::Image& newSource);

        // Render the image from cache
        juce::Image& render();

        void setRadius (size_t newRadius);

    private:
        // juce::Images are value objects, reference counted behind the scenes
        // We want to store a reference to the src so we can compare on render
        // And we actually are the owner of the dst
        size_t radius = 0;
        juce::Image src {};
        juce::Image dst {};
        bool needsRedraw = false;
    };
}
