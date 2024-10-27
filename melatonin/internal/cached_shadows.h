#pragma once
#include "rendered_single_channel_shadow.h"

namespace melatonin::internal
{
    // This class isn't meant for direct usage and may change its API over time!
    // Use DropShadow and InnerShadow and PathWithShadows instead, you've been warned...
    class CachedShadows
    {
    protected:
        CachedShadows() = default;
        CachedShadows (const CachedShadows&) = delete;
        CachedShadows& operator= (const CachedShadows&) = default;
        CachedShadows (CachedShadows&&) = delete;
        CachedShadows& operator= (CachedShadows&&) = default;

        // allow us to just pass multiple radii to get multiple shadows
        CachedShadows (std::initializer_list<int> radii, bool isInner = false);
        CachedShadows (std::initializer_list<double> radii, bool isInner = false);

        // multiple shadows
        CachedShadows (std::initializer_list<ShadowParametersInt> shadowParameters, bool force_inner = false);
        explicit CachedShadows (const std::vector<ShadowParametersInt>& shadowParameters, bool force_inner = false);

        virtual ~CachedShadows() = default;

    public:
        // store a copy of the path to compare against for caching
        // public for testability, sorry not sorry
        // too lazy to break out ARGBComposite into its own class
        juce::Path lastOriginAgnosticPath = {};
        juce::Path lastOriginAgnosticPathScaled = {};

        void render (juce::Graphics& g, const juce::Path& newPath, bool lowQuality = false);
        void render (juce::Graphics& g, const juce::Path& newPath, const juce::PathStrokeType& newType, bool lowQuality = false);
        void render (juce::Graphics& g, const juce::String& text, const juce::Rectangle<float>& area, juce::Justification justification);
        void render (juce::Graphics& g, const juce::String& text, const juce::Rectangle<int>& area, juce::Justification justification);
        void render (juce::Graphics& g, const juce::String& text, int x, int y, int width, int height, juce::Justification justification);

        // these are float that will round, ints will implicitly convert
        CachedShadows& setRadius (double radius, size_t index = 0);
        CachedShadows& setSpread (double spread, size_t index = 0);

        // accept int offset, this is what {1, 1} will default to
        // without it, the compiler will fail to pick a type
        CachedShadows& setOffset (juce::Point<int> offset, size_t index = 0);

        // accept raw initializer list for juce::Point float types like {1.0, 1.0}
        // without intercepting the list, the compiler tries a float->int narrowing conversion!
        template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
        CachedShadows& setOffset (std::initializer_list<T> offset, size_t index = 0)
        {
            jassert (offset.size() == 2);
            auto it = offset.begin();
            T x = *it++;
            T y = *it;
            return setOffset (juce::Point { juce::roundToInt (x), juce::roundToInt (y) }, index);
        }

        // accept all Point types and round
        // Use SFINAE to force int type getting the priority with literals like {1, 1}
        // This allows setOffset({1, 1}) to not be ambiguous
        // Overload for juce::Point<int>
        template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
        CachedShadows& setOffset (const juce::Point<T> offset, size_t index = 0)
        {
            return setOffset (offset.toInt(), index);
        }

        // setOffset(1, 1) will default to int
        CachedShadows& setOffset (int x, int y, size_t index = 0)
        {
            return setOffset (juce::Point<int> { x, y }, index);
        }

        // setOffset(1.0, 1.0) and float variant work as expected
        template <typename T>
        CachedShadows& setOffset (T x, T y, size_t index = 0)
        {
            return setOffset (juce::Point<int> { juce::roundToInt(x), juce::roundToInt(y) }, index);
        }

        CachedShadows& setColor (juce::Colour color, size_t index = 0);
        CachedShadows& setOpacity (double opacity, size_t index = 0);

        // helps with testing and debugging cache
        [[nodiscard]] bool willRecalculate() const { return needsRecalculate; }
        [[nodiscard]] bool willRecomposite() const { return needsRecomposite; }

    protected:
        // TODO: Is there a better pattern here?
        // InnerShadow must set inner=true
        // Maybe "inner" is better as a member of CachedShadows vs. ShadowParametersInt?
        virtual ShadowParametersInt emptyShadow()
        {
            return ShadowParametersInt {};
        }

        template <typename T>
        static std::vector<ShadowParametersInt> convertToIntParameters (std::initializer_list<ShadowParameters<T>> params)
        {
            std::vector<ShadowParametersInt> intParams;
            intParams.reserve (params.size());
            for (const auto& p : params)
            {
                intParams.push_back (ShadowParametersInt (p));
            }
            return intParams;
        }

    private:
        // any float offset from 0,0 the path has is stored here
        juce::Point<float> pathPositionInContext = {};

        // this stores the final, end result
        juce::Image compositedARGB;
        juce::Point<float> scaledCompositePosition;

        // each component blur is stored here, their positions are stored in ShadowParametersInt
        std::vector<RenderedSingleChannelShadow> renderedSingleChannelShadows;

        // if radius/spread stay the same, we can reuse the blur
        bool needsRecalculate = true;

        // this lets us adjust color/opacity without re-rendering blurs
        bool needsRecomposite = true;

        float scale = 1.0;

        bool stroked = false;
        juce::PathStrokeType strokeType { -1.0f };

        struct TextArrangement
        {
            juce::String text;
#if JUCE_MAJOR_VERSION >= 8
            juce::Font font = juce::FontOptions {};
#else
            juce::Font font;
#endif
            juce::Rectangle<float> area;
            juce::Justification justification = juce::Justification::left;

            bool operator== (const TextArrangement& other) const;
            bool operator!= (const TextArrangement& other) const;
        };

        TextArrangement lastTextArrangement = {};

        bool canUpdateShadow (size_t index);
        void setScale (juce::Graphics& g, bool lowQuality);
        void updatePathIfNeeded (juce::Path& pathToBlur);
        void recalculateBlurs();
        void renderInternal (juce::Graphics& g);
        void drawARGBComposite (juce::Graphics& g, bool optimizeClipBounds = false);

        // This is done at the main graphics context scale
        // The path is at 0,0 and the shadows are placed at their correct relative *integer* positions
        void compositeShadowsToARGB();
    };
}
