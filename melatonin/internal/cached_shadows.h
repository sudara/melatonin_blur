#pragma once
#include "rendered_single_channel_shadow.h"

namespace melatonin::internal
{
    // This class isn't meant for direct usage and may change its API over time!
    // Use DropShadow and InnerShadow and PathWithShadows instead, you've been warned...
    class CachedShadows
    {
    protected:
        CachedShadows (std::initializer_list<ShadowParameters> shadowParameters, bool force_inner = false);

    public:
        // store a copy of the path to compare against for caching
        // public for testability, sorry not sorry
        // too lazy to break out ARGBComposite into its own class
        juce::Path lastOriginAgnosticPath = {};
        juce::Path lastOriginAgnosticPathScaled = {};

        void render (juce::Graphics& g, const juce::Path& newPath, bool lowQuality = false);
        void renderStroked (juce::Graphics& g, const juce::Path& newPath, const juce::PathStrokeType& newType, bool lowQuality = false);

        void setRadius (size_t radius, size_t index = 0);
        void setSpread (size_t spread, size_t index = 0);
        void setOffset (juce::Point<int> offset, size_t index = 0);
        void setColor (juce::Colour color, size_t index = 0);
        void setOpacity (float opacity, size_t index = 0);

    private:
        // any float offset from 0,0 the path has is stored here
        juce::Point<float> pathPositionInContext = {};

        // this stores the final, end result
        juce::Image compositedARGB;
        juce::Point<float> scaledCompositePosition;

        // each component blur is stored here, their positions are stored in ShadowParameters
        std::vector<RenderedSingleChannelShadow> renderedSingleChannelShadows;

        // this lets us adjust color/opacity without re-rendering blurs
        bool needsRecomposite = true;
        bool needsRecalculate = true;

        float lastScale = 1.0;

        bool stroked = false;
        juce::PathStrokeType strokeType { -1.0f };

        void recalculateBlurs (float scale);

        void drawARGBComposite (juce::Graphics& g, float scale, bool optimizeClipBounds = false);

        // This is done at the main graphics context scale
        // The path is at 0,0 and the shadows are placed at their correct relative *integer* positions
        void compositeShadowsToARGB();
    };
}
