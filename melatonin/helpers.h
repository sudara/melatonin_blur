#pragma once

#include "blur.h"
namespace melatonin
{
    // these are the parameters required to represent a single drop or inner shadow
    struct ShadowParameters
    {
        // one single color per shadow
        const juce::Colour color = {};
        const int radius = 1;
        const juce::Point<int> offset = { 0, 0 };

        // Spread literally just expands or contracts the path size
        // Inverted for inner shadows
        const int spread = 0;

        // an inner shadow is just a modified drop shadow
        bool inner = false;

        // each shadow takes up a different amount of space depending on it's radius, spread, etc
        juce::Rectangle<int> area = {};
    };

    // this caches the expensive shadow creation into a ARGB juce::Image for fast compositing
    static inline juce::Image renderShadowToARGB (ShadowParameters& s, juce::Path& originalPath)
    {
        // the area of each cached blur depends on its radius and spread
        s.area = (originalPath.getBounds().getSmallestIntegerContainer() + s.offset)
                     .expanded (s.radius + s.spread + 1);

        // TODO: Investigate/test what this line does — makes the clip smaller for certain cases?
        //.getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

        // Reconsider your parameters: one of the dimensions is 0 so the blur doesn't exist!
        if (s.area.getWidth() < 1 || s.area.getHeight() < 1)
            jassertfalse;

        // we don't want to modify our original path (it would break cache)
        // additionally, inner shadows must render a modified path
        auto shadowPath = juce::Path (originalPath);

        if (s.spread != 0)
        {
            // TODO: drop shadow tests for s.area to understand why this is still needed (we expanded above!)
            s.area.expand (s.spread, s.spread);
            auto bounds = originalPath.getBounds().expanded (s.inner ? (float) -s.spread : (float) s.spread);
            shadowPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        // inner shadows are rendered by inverting the path, drop shadowing and clipping to the original path
        if (s.inner)
        {
            shadowPath.setUsingNonZeroWinding (false);
            shadowPath.addRectangle (s.area.expanded (10));
        }

        // each shadow is its own single channel image associated with a color
        juce::Image renderedSingleChannel (juce::Image::SingleChannel, s.area.getWidth(), s.area.getHeight(), true);

        // boot up another graphics context to give us access to fillPath, etc
        {
            juce::Graphics g2 (renderedSingleChannel);

            g2.setColour (juce::Colours::white);
            g2.fillPath (shadowPath, juce::AffineTransform::translation ((float) (s.offset.x - s.area.getX()), (float) (s.offset.y - s.area.getY())));
        }

        // perform the blur with the fastest algorithm available
        melatonin::blur::singleChannel (renderedSingleChannel, s.radius);

        // YET ANOTHER graphics context to efficiently convert the image to ARGB
        // why? Because later, compositing to the main graphics context becomes (g) faster
        // (don't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
        // which slows down the main compositing by a factor of 2-3x)
        // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76
        juce::Image renderedARGB (juce::Image::ARGB, s.area.getWidth(), s.area.getHeight(), true);
        {
            juce::Graphics g2 (renderedARGB);
            g2.setColour (s.color);
            g2.drawImageAt (renderedSingleChannel, 0, 0, true);
        }
        return renderedARGB;
    }

    // This class isn't meant for direct usage! Use DropShadow and InnerShadow
    class CachedShadow
    {
    protected:
        std::vector<ShadowParameters> shadowParameters;
        CachedShadow (std::initializer_list<ShadowParameters> p) : shadowParameters (p)
        {
            jassert (!shadowParameters.empty());

            for (auto& shadow : shadowParameters)
            {
                // 0 radius means no shadow..
                if (shadow.radius < 1)
                {
                    jassertfalse;
                    continue;
                }

                // each shadow is backed by a JUCE image
                renderedShadows.emplace_back();
            }
        }

    public:
        void render (juce::Graphics& g, const juce::Path& newPath, bool optimizeClipBounds = false)
        {
            // recalculate blurs only when the path changes (otherwise render from cache)
            // TODO: The path is stored in this class, probably not necessary/efficient
            // Can it be replaced with a hashing mechanism?
            if (newPath != path)
            {
                path = newPath;
                recalculateBlurs();
            }

            drawCachedBlurs (g, optimizeClipBounds);
        }

#if JUCE_MAC
        // currently unused, may be benchmarked vs. drawImageAt
        static juce::Image convertToARGB (juce::Image& src, juce::Colour color)
        {
            jassert (src.getFormat() == juce::Image::SingleChannel);
            juce::Image dst (juce::Image::ARGB, src.getWidth(), src.getHeight(), true);
            juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);
            vImage_Buffer alphaBuffer = { srcData.getLinePointer (0), static_cast<vImagePixelCount> (src.getHeight()), static_cast<vImagePixelCount> (src.getWidth()), static_cast<size_t> (srcData.lineStride) };
            vImage_Buffer dstBuffer = { dstData.getLinePointer (0), static_cast<vImagePixelCount> (dst.getHeight()), static_cast<vImagePixelCount> (dst.getWidth()), static_cast<size_t> (dstData.lineStride) };

            // vdsp doesn't have a Planar8toBGRA function, so we just shuffle the channels manually
            // (and assume we're always little endian)
            vImageConvert_Planar8toARGB8888 (&alphaBuffer, &alphaBuffer, &alphaBuffer, &alphaBuffer, &dstBuffer, kvImageNoFlags);
            vImageOverwriteChannelsWithScalar_ARGB8888 (color.getRed(), &dstBuffer, &dstBuffer, 0x2, kvImageNoFlags);
            vImageOverwriteChannelsWithScalar_ARGB8888 (color.getGreen(), &dstBuffer, &dstBuffer, 0x4, kvImageNoFlags);
            vImageOverwriteChannelsWithScalar_ARGB8888 (color.getBlue(), &dstBuffer, &dstBuffer, 0x8, kvImageNoFlags);

            // BGRA = little endian ARGB
            vImagePremultiplyData_BGRA8888 (&dstBuffer, &dstBuffer, kvImageNoFlags);
            return dst;
        }
#endif

    private:
        juce::Path path;
        std::vector<juce::Image> renderedShadows;

        void recalculateBlurs()
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];
                renderedShadows[i] = renderShadowToARGB (s, path);
            }
        }

        void drawCachedBlurs (juce::Graphics& g, bool optimizeClipBounds = false)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];

                // resets the Clip Region when this scope ends
                juce::Graphics::ScopedSaveState saveState (g);

                // for inner shadows we don't want anything outside the path bounds
                if (s.inner)
                    g.reduceClipRegion (path);
                else if (optimizeClipBounds)
                {
                    // don't bother drawing what's inside the path's bounds
                    // TODO: requires testing/benchmarking
                    g.excludeClipRegion (path.getBounds().toNearestIntEdges());
                }

                // Not sure why, but this is required despite fillAlphaChannelWithCurrentBrush=false
                g.setColour (s.color);

                // Specifying `false` for `fillAlphaChannelWithCurrentBrush` here
                // is a 2-3x speedup on the actual image rendering
                g.drawImageAt (renderedShadows[i], s.area.getX(), s.area.getY());
            }
        }
    };
}
