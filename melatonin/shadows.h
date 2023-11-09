#pragma once
#include "helpers.h"
#include "implementations/gin.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_blur/melatonin/blur.h"

namespace melatonin
{
    // A drop shadow is a path filled by a single color and then blurred.
    // These shadows are cached.
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
            std::for_each (shadowParameters.begin(), shadowParameters.end(), [](auto& s) { s.inner = true; });
        }
    };
}
