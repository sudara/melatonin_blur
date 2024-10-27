#pragma once
#include "internal/cached_shadows.h"

namespace melatonin
{
    /*  A drop shadow is a path filled by a single color and then blurred.
        These shadows are cached.

        Both DropShadow and InnerShadow take the same parameters and should be
        held as a class member of a juce::Component:

        melatonin::DropShadow shadow = {{ juce::Colours::black, 8, { -2, 0 }, 2 }};

        ShadowParametersInt is a struct that looks like this:

        struct ShadowParametersInt
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

        // allow us to just pass a radius to get a black shadow
        explicit DropShadow (const int radius) : CachedShadows ({ { juce::Colours::black, radius } }) {}
        explicit DropShadow (const float radius) : CachedShadows ({ { juce::Colours::black, juce::roundToInt (radius) } }) {}

        // allow us to just pass multiple radii to get multiple shadows
        DropShadow (std::initializer_list<int> radii) : CachedShadows (radii) {}

        // individual arguments
        DropShadow (const juce::Colour color, const int radius, const juce::Point<int> offset = { 0, 0 }, const int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }) {}

        // randall-style float arguments (these get rounded)
        DropShadow (const juce::Colour color, const float radius, const juce::Point<float> offset = { 0, 0 }, const float spread = 0)
            : CachedShadows ({ { color, juce::roundToInt (radius), { juce::roundToInt (offset.x), juce::roundToInt (offset.y) }, juce::roundToInt (spread) } }) {}

        // single ShadowParameters with integer arguments
        // melatonin::DropShadow ({Colour::fromRGBA (255, 183, 43, 111), pulse (6)}).render (g, button);
        explicit DropShadow (ShadowParametersInt p) : CachedShadows ({ p }) {}

        // single ShadowParameters with float/double argumuents
        template <typename T>
        explicit DropShadow (const ShadowParameters<T>& p) : CachedShadows ({ p })
        {}

        // multiple ShadowParametersInt
        DropShadow (std::initializer_list<ShadowParametersInt> p) : CachedShadows (p) {}

        // multiple via a std::vector
        DropShadow (const std::vector<ShadowParametersInt>& p) : CachedShadows (p) {}
    };

    // An inner shadow is basically the *inverted* filled path, blurred and clipped to the path
    // so the blur is only visible *inside* the path.
    class InnerShadow : public internal::CachedShadows
    {
    public:
        InnerShadow() = default;

        // allow us to just pass a radius to get a black shadow
        explicit InnerShadow (const int radius) : CachedShadows ({ { juce::Colours::black, radius } }, true) {}
        explicit InnerShadow (const float radius) : CachedShadows ({ { juce::Colours::black, juce::roundToInt (radius) } }, true) {}

        // allow us to just pass multiple radii to get multiple shadows
        InnerShadow (std::initializer_list<int> radii) : CachedShadows (radii, true) {}

        // individual arguments
        InnerShadow (const juce::Colour color, const int radius, const juce::Point<int> offset = { 0, 0 }, const int spread = 0)
            : CachedShadows ({ { color, radius, offset, spread } }, true) {}

        // randall-style float arguments
        InnerShadow (const juce::Colour color, const float radius, const juce::Point<float> offset = { 0, 0 }, const float spread = 0)
            : CachedShadows ({ { color, juce::roundToInt (radius), { juce::roundToInt (offset.x), juce::roundToInt (offset.y) }, juce::roundToInt (spread) } }, true) {}

        // single
        explicit InnerShadow (ShadowParametersInt p) : CachedShadows ({ p }, true) {}

        // multiple shadows
        InnerShadow (std::initializer_list<ShadowParametersInt> p) : CachedShadows (p, true) {}

        // multiple via a std::vector
        explicit InnerShadow (const std::vector<ShadowParametersInt>& p) : CachedShadows (p, true) {}

    private:
        ShadowParametersInt emptyShadow() override
        {
            auto empty = ShadowParametersInt {};
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
        PathWithShadows (std::initializer_list<ShadowParametersInt> p)
        {
            std::vector<ShadowParametersInt> innerParameters;
            std::vector<ShadowParametersInt> dropParameters;
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
