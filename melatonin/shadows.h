#pragma once
#include "implementations/gin.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "support/cached_shadow.h"

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
            const juce::Colour color = {};
            const int radius = 1;
            const juce::Point<int> offset = { 0, 0 };

            // Spread expands or contracts the path size
            // Inverted for inner shadows
            const int spread = 0;
        }
    */
    class DropShadow : public CachedShadow
    {
    public:
        DropShadow (std::initializer_list<ShadowParameters> p) : CachedShadow (p) {}
    };

    // An inner shadow is basically the *inverted* filled path, blurred and clipped to the path
    // so the blur is only visible *inside* the path.
    class InnerShadow : public CachedShadow
    {
    public:
        InnerShadow (std::initializer_list<ShadowParameters> p) : CachedShadow (p)
        {
            std::for_each (shadowParameters.begin(), shadowParameters.end(), [] (auto& s) { s.inner = true; });
        }
    };
}
