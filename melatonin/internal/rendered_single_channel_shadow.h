#pragma once
#include "juce_gui_basics/juce_gui_basics.h"

namespace melatonin
{
    // these are the parameters required to represent a single drop or inner shadow
    // wish I could put these in shadows.h to help people
    template <typename ValueType = int>
    struct ShadowParameters
    {
        // one single color per shadow
        juce::Colour color = juce::Colours::black;
        ValueType radius = 1;
        juce::Point<ValueType> offset = { 0, 0 };

        // Spread literally just expands or contracts the *path* size
        // Inverted for inner shadows
        ValueType spread = 0;

        // an inner shadow is just a modified drop shadow
        bool inner = false;

        // Needed for aggregate-style initialization
        ShadowParameters (juce::Colour c, ValueType r, juce::Point<ValueType> o = {}, ValueType s = 0, bool i = false)
            : color (c), radius (r), offset (o), spread (s), inner(i) {}

        // round all non-int types
        template <typename OtherType>
        explicit ShadowParameters (const ShadowParameters<OtherType>& other)
            : color (other.color),
              radius (juce::roundToInt (other.radius)),
              offset (other.roundToInt()),
              spread (juce::roundToInt (other.spread)),
              inner (other.inner)
        {}

        ShadowParameters() = default;
    };

    using ShadowParametersInt = ShadowParameters<>;

    namespace internal
    {
        // encapsulates logic for rendering a path to inner/drop shadow
        // the image is optimized to be as small as possible
        // the path is always 0,0 in the image
        class RenderedSingleChannelShadow
        {
        public:
            ShadowParametersInt parameters;

            explicit RenderedSingleChannelShadow (ShadowParametersInt p);

            juce::Image& render (juce::Path& originAgnosticPath, float scale, bool stroked = false);

            // Offset is added on the fly, it's not actually a part of the render
            // and can change without invalidating cache
            juce::Rectangle<int> getScaledBounds();
            juce::Rectangle<int> getScaledPathBounds();

            [[nodiscard]] const juce::Image& getImage();

            [[nodiscard]] bool updateRadius (int radius);
            [[nodiscard]] bool updateSpread (int spread);
            [[nodiscard]] bool updateOffset (juce::Point<int> offset, float scale);
            [[nodiscard]] bool updateColor (juce::Colour color);
            [[nodiscard]] bool updateOpacity (float opacity);

            // this doesn't re-render, just re-calculates position stuff
            void updateScaledShadowBounds (float scale);

        private:
            juce::Image singleChannelRender;
            juce::Rectangle<int> scaledShadowBounds;
            juce::Rectangle<int> scaledPathBounds;

            int scaledRadius = 0;
            int scaledSpread = 0;

            // Offsets are separately stored to translate placement in ARGB compositing.
            juce::Point<int> scaledOffset;
        };
    }
}
