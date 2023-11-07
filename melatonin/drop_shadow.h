#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "melatonin_blur/melatonin/blur.h"
#include "implementations/gin.h"

namespace melatonin
{
    class DropShadow
    {
    public:
        // these are the parameters required to represent a single drop shadow
        struct Parameters
        {
            const juce::Colour color;
            const int radius = 1;
            const juce::Point<int> offset = { 0, 0 };

            // A positive value increases the size of the shadow
            // a negative value decreases the size of the shadow
            const int spread = 0;

            juce::Rectangle<int> area = {};
        };

#if JUCE_MAC
        static juce::Image convertToARGB (juce::Image& src, juce::Colour color)
        {
            jassert (src.getFormat() == juce::Image::SingleChannel);
            juce::Image dst (juce::Image::ARGB, src.getWidth(), src.getHeight(), true);
            //            juce::Graphics g (dst);
            //            g.fillAll (juce::Colours::white);
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

        DropShadow (std::initializer_list<Parameters> p) : shadowParameters (p)
        {
            // each shadow is a JUCE image (each with have its own color)
            for (auto& shadow : shadowParameters)
            {
                // 0 radius means no shadow..
                if (shadow.radius < 1)
                {
                    jassertfalse;
                    continue;
                }
                renderedShadows.emplace_back();
            }
        }

        void render (juce::Graphics& g, const juce::Path& newPath, bool optimizeClipBounds = false)
        {
            jassert (!shadowParameters.empty());

            // recalculate blurs when the path changes (otherwise render from cache)
            if (newPath != path)
            {
                path = newPath;
                recalculateBlurs (g);
            }

            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];

                // resets the Clip Region when this scope ends
                juce::Graphics::ScopedSaveState saveState (g);

                // exclude the biggest rectangle in the path
                if (optimizeClipBounds)
                {
                    g.excludeClipRegion (path.getBounds().toNearestIntEdges());
                }

                // renderedShadows should already be composited as ARGB
                // This lets us specify `false` for `fillAlphaChannelWithCurrentBrush` here
                // Which is a 2-3x speedup
                g.setOpacity(1.0);
                g.drawImageAt (renderedShadows[i], s.area.getX(), s.area.getY());
            }
        }

    private:
        juce::Path path;
        std::vector<Parameters> shadowParameters;
        std::vector<juce::Image> renderedShadows;

        void recalculateBlurs (juce::Graphics& g)
        {
            for (size_t i = 0; i < shadowParameters.size(); ++i)
            {
                auto& s = shadowParameters[i];

                // the area of each shadow depends on its radius and spread
                s.area = (path.getBounds().getSmallestIntegerContainer() + s.offset)
                             .expanded (s.radius + s.spread + 1)
                             .getIntersection (g.getClipBounds().expanded (s.radius + s.spread + 1));

                // if the area is too small, don't bother
                if (s.area.getWidth() < 1 || s.area.getHeight() < 1)
                    return;

                // "spread" enlarges or shrinks the path before blurring it
                auto spreadPath = juce::Path (path);
                if (s.spread != 0)
                {
                    s.area.expand (s.spread, s.spread);
                    auto bounds = path.getBounds().expanded ((float) s.spread);
                    spreadPath.scaleToFit (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
                }

                // each shadow is its own single channel image associated with a color
                juce::Image renderedSingleChannel (juce::Image::SingleChannel, s.area.getWidth(), s.area.getHeight(), true);

                // boot up another graphics context to give us access to fillPath, etc
                {
                    juce::Graphics g2 (renderedSingleChannel);
                    g2.setColour (juce::Colours::white);
                    g2.fillPath (spreadPath, juce::AffineTransform::translation ((float) (s.offset.x - s.area.getX()), (float) (s.offset.y - s.area.getY())));
                }
                melatonin::blur::singleChannel (renderedSingleChannel, s.radius);

                // and YET ANOTHER graphics context to efficiently convert the image to ARGB
                // why? Because it makes compositing to the main graphics context (g) faster
                // (don't need to specify `fillAlphaChannelWithCurrentBrush` for `drawImageAt`,
                // which slows down the main compositing by a factor of 2-3x)
                // see: https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76

                // renderedShadows[i] = convertToARGB (renderedSingleChannel, s.color);

                juce::Image renderedARGB (juce::Image::ARGB, s.area.getWidth(), s.area.getHeight(), true);
                {
                    juce::Graphics g2 (renderedARGB);
                    g2.setColour (s.color);
                    g2.drawImageAt (renderedSingleChannel, 0, 0, true);
                }
                renderedShadows[i] = renderedARGB;
            }
        }
    };
}
