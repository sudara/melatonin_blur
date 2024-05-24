#pragma once
#include "implementations/gin.h"
#include "internal/cached_shadows.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin
{
    /*  A drop shadow is a path filled by a single color and then blurred.
        These shadows are cached.

        Both DropShadow and InnerShadow take the same parameters and should be
        held as a class member of a juce::Component:

        melatonin::DropShadow shadow = {{ juce::Colours::black, 8, { -2, 0 }, 2 }};

        ShadowParameters is a struct that looks like this:

        struct ShadowParameters
        {
            // one single color per shadow
            juce::Colour color = {};
            int radius = 1;
            juce::Point<int> offset = { 0, 0 };

            // Spread literally just expands or contracts the *path* size
            // Inverted for inner shadows
            int spread = 0;

            // an inner shadow is just a modified drop shadow
            bool inner = false;
        };
    */
    class DropShadow : public internal::CachedShadows
    {
    public:
        DropShadow() = default;

        // individual arguments
        DropShadow (juce::Colour color, int radius, juce::Point<int> offset = { 0, 0 }, int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }) {}

        // single ShadowParameters
        // melatonin::DropShadow ({Colour::fromRGBA (255, 183, 43, 111), pulse (6)}).render (g, button);
        explicit DropShadow (ShadowParameters p) : CachedShadows ({ p }) {}

        // multiple ShadowParameters
        DropShadow (std::initializer_list<ShadowParameters> p) : CachedShadows (p) {}

        // multiple via a std::vector
        DropShadow (const std::vector<ShadowParameters>& p) : CachedShadows (p) {}
    };

    // An inner shadow is basically the *inverted* filled path, blurred and clipped to the path
    // so the blur is only visible *inside* the path.
    class InnerShadow : public internal::CachedShadows
    {
    public:
        InnerShadow() = default;

        // individual arguments
        InnerShadow (juce::Colour color, int radius, juce::Point<int> offset = { 0, 0 }, int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }, true) {}

        // single
        explicit InnerShadow (ShadowParameters p) : CachedShadows ({ p }, true) {}

        // multiple shadows
        InnerShadow (std::initializer_list<ShadowParameters> p) : CachedShadows (p, true) {}

        // multiple via a std::vector
        InnerShadow (const std::vector<ShadowParameters>& p) : CachedShadows (p, true) {}

    private:
        ShadowParameters emptyShadow() override
        {
            auto empty = ShadowParameters {};
            empty.inner = true;
            return empty;
        }
    };

    // Owns and renders a path plus a collection of inner and drop shadows
    // great for buttons, modals, etc...
    class PathWithShadows
    {
    public:
        juce::Path path;

        PathWithShadows() = default;

        // multiple shadows
        PathWithShadows (std::initializer_list<ShadowParameters> p)
        {
            std::vector<ShadowParameters> innerParameters;
            std::vector<ShadowParameters> dropParameters;
            for (const auto& param : p)
            {
                if (param.inner)
                {
                    innerParameters.push_back (param);
                }
                else
                {
                    dropParameters.push_back (param);
                }
            }

            dropShadow = DropShadow (dropParameters);
            innerShadow = InnerShadow (innerParameters);
        }

        void render (juce::Graphics& g)
        {
            dropShadow.render (g, path);
            g.fillPath (path);
            innerShadow.render (g, path);
        }

        void render (juce::Graphics& g, const juce::PathStrokeType& strokeType)
        {
            dropShadow.render (g, path, strokeType);
            g.strokePath (path, strokeType);
            innerShadow.render (g, path, strokeType);
        }

        DropShadow dropShadow;
        InnerShadow innerShadow;
    };
}
