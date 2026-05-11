#pragma once

#if MELATONIN_BLUR_USE_DIRECT2D
 #include <catch2/generators/catch_generators_all.hpp>

struct ScopedDirect2DSetting
{
    ScopedDirect2DSetting() : previous (melatonin::blur::isDirect2DEnabled()) {}

    ~ScopedDirect2DSetting()
    {
        melatonin::blur::setDirect2DEnabled (previous);
    }

    bool previous = false;
};

 #define MELATONIN_BLUR_TEST_EACH_DIRECT2D_MODE() \
     const ScopedDirect2DSetting resetDirect2D; \
     melatonin::blur::setDirect2DEnabled (GENERATE (false, true)); \
     CAPTURE (melatonin::blur::isDirect2DEnabled())
#else
 #define MELATONIN_BLUR_TEST_EACH_DIRECT2D_MODE() ((void) 0)
#endif

// We can't rely on JUCE's Colour class for un-premultiplied truth
// (and don't have access to internals)
// so lets roll our own access to the data
struct ActualPixel
{
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// it's just tests, take off your safety helmet and get dangerous with me!
// macOS and windows are Little Endian, so unfortunately ARGB is stored "backwards" as BGRA
[[nodiscard]] [[maybe_unused]] inline ActualPixel getActualARGBPixel (uint8_t* jucePixel)
{
#if JUCE_BIG_ENDIAN
    return { jucePixel[0], jucePixel[1], jucePixel[2], jucePixel[3] };
#else
    return { jucePixel[3], jucePixel[2], jucePixel[1], jucePixel[0] };
#endif
}

inline void setActualPixel (uint8_t* jucePixel, ActualPixel actualPixel)
{
#if JUCE_BIG_ENDIAN
    jucePixel[0] = actualPixel.a;
    jucePixel[1] = actualPixel.r;
    jucePixel[2] = actualPixel.g;
    jucePixel[3] = actualPixel.b;
#else
    jucePixel[3] = actualPixel.a;
    jucePixel[2] = actualPixel.r;
    jucePixel[1] = actualPixel.g;
    jucePixel[0] = actualPixel.b;
#endif
}

// ugly, but makes testing more readable via float arrays
[[maybe_unused]] static std::vector<float> pixelRow (const juce::Image& image, int row, int channel = -1)
{
    auto singleChannel = image.getFormat() == juce::Image::PixelFormat::SingleChannel;
    std::vector<float> result;
    juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);
    for (auto x = 0; x < image.getWidth(); ++x)
    {
        if (singleChannel)
        {
            auto color = data.getPixelColour (x, row);
            result.push_back (color.getBrightness());
        }
        else
        {
            auto pixel = getActualARGBPixel (data.getPixelPointer (x, row));

            // assume little endian (aka BGRA)
            if (channel == 0)
                result.push_back ((float) pixel.b / 255.0f);
            else if (channel == 1)
                result.push_back ((float) pixel.g / 255.0f);
            else if (channel == 2)
                result.push_back ((float) pixel.r / 255.0f);
            else if (channel == 3)
                result.push_back (data.getPixelColour (x, row).getAlpha());
            else
                jassertfalse;
        }
    }
    return result;
}
[[maybe_unused]] static std::vector<float> pixelCol (const juce::Image& image, int col, int channel = -1)
{
    std::vector<float> result;
    juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);
    for (auto y = 0; y < image.getHeight(); ++y)
    {
        if (image.getFormat() == juce::Image::PixelFormat::SingleChannel)
            result.push_back (data.getPixelColour (col, y).getBrightness());
        else
        {
            auto pixel = getActualARGBPixel (data.getPixelPointer (col, y));

            // assume little endian (aka BGRA)
            if (channel == 0)
                result.push_back ((float) pixel.b / 255.0f);
            else if (channel == 1)
                result.push_back ((float) pixel.g / 255.0f);
            else if (channel == 2)
                result.push_back ((float) pixel.r / 255.0f);
            else if (channel == 3)
                result.push_back (data.getPixelColour (col, y).getAlpha());
            else
                jassertfalse;
        }
    }
    return result;
}

[[maybe_unused]] static juce::String getPixel (juce::Image& img, int x, int y)
{
    return img.getPixelAt (x, y).toDisplayString (true);
}

[[maybe_unused]] static float getScaledBrightness (juce::Image& img, int x, int y, float scale)
{
    x = juce::roundToInt ((float) x * scale);
    y = juce::roundToInt ((float) y * scale);
    return img.getPixelAt (x, y).getBrightness();
}

// get pixels in a range, *includes* the start/end of range
[[maybe_unused]] static juce::String getPixels (juce::Image& img, int x, juce::Range<int> yRange)
{
    juce::String result;
    for (auto y = yRange.getStart(); y <= yRange.getEnd(); ++y)
    {
        result << getPixel (img, x, y);
        if (y != yRange.getEnd())
            result << ", ";
    }
    return result;
}

[[maybe_unused]] static juce::String getPixels (juce::Image& img, juce::Range<int> xRange, int y)
{
    juce::String result;
    for (auto x = xRange.getStart(); x <= xRange.getEnd(); ++x)
    {
        result << getPixel (img, x, y);
        if (x != xRange.getEnd())
            result << ", ";
    }
    return result;
}

[[maybe_unused]] static juce::String getPixels (juce::Image& img, juce::Range<int> xRange, juce::Range<int> yRange)
{
    juce::String result;
    for (auto y = yRange.getStart(); y <= yRange.getEnd(); ++y)
    {
        for (auto x = xRange.getStart(); x <= xRange.getEnd(); ++x)
        {
            result << getPixel (img, x, y);
            if (!(x == xRange.getEnd() && y == yRange.getEnd()))
                result << ", ";
        }
    }
    return result;
}

[[maybe_unused]] static std::vector<float> getPixelsBrightness (juce::Image& img, int x, juce::Range<int> yRange)
{
    std::vector<float> result;
    for (auto y = yRange.getStart(); y <= yRange.getEnd(); ++y)
    {
        result.push_back (img.getPixelAt (x, y).getBrightness());
    }
    return result;
}

[[maybe_unused]] static std::vector<float> getPixelsBrightness (juce::Image& img, juce::Range<int> xRange, int y)
{
    std::vector<float> result;
    for (auto x = xRange.getStart(); x <= xRange.getEnd(); ++x)
    {
        result.push_back (img.getPixelAt (x, y).getBrightness());
    }
    return result;
}

[[maybe_unused]] static bool isImageFilled (const juce::Image& img, const juce::Colour& color)
{
    const juce::Image::BitmapData data (img, juce::Image::BitmapData::readOnly);
    for (auto y = 0; y < img.getHeight(); ++y)
    {
        for (auto x = 0; x < img.getWidth(); ++x)
        {
            auto pixelColor = data.getPixelColour (x, y);

            // TODO: again, windows compositing seems to need some leeway
            // Here, the alpha channel on the far right is 253 instead of 255
            if (!juce::approximatelyEqual (pixelColor.getFloatRed(), color.getFloatRed())
                || !juce::approximatelyEqual (pixelColor.getFloatGreen(), color.getFloatGreen())
                || !juce::approximatelyEqual (pixelColor.getFloatBlue(), color.getFloatBlue())
                || std::abs (pixelColor.getFloatAlpha() - color.getFloatAlpha()) > 0.01f)
                return false;
        }
    }
    return true;
}

// tests all are on WHITE
// so filled bounds have any other color than pure white
[[maybe_unused]] static juce::Rectangle<int> filledBounds (juce::Image& img)
{
    juce::Rectangle<int> result;
    juce::Image::BitmapData data (img, juce::Image::BitmapData::readOnly);
    for (auto y = 0; y < img.getHeight(); ++y)
    {
        for (auto x = 0; x < img.getWidth(); ++x)
        {
            if (data.getPixelColour (x, y) != juce::Colours::white)
            {
                result = result.getUnion (juce::Rectangle<int> (x, y, 1, 1));
            }
        }
    }
    return result;
}

[[maybe_unused]] static bool imagesAreIdentical (juce::Image& img1, juce::Image& img2)
{
    if (img1.getBounds() != img2.getBounds() || img1.getFormat() != img2.getFormat())
        return false;

    juce::Image::BitmapData data1 (img1, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData data2 (img2, juce::Image::BitmapData::readOnly);
    for (auto y = 0; y < img1.getHeight(); ++y)
    {
        for (auto x = 0; x < img1.getWidth(); ++x)
        {
            if (data1.getPixelColour (x, y) != data2.getPixelColour (x, y))
                return false;
        }
    }
    return true;
}

[[maybe_unused]] static bool imagesAreIdenticalWithTolerance (juce::Image& img1, juce::Image& img2, uint8_t tolerance)
{
    if (img1.getBounds() != img2.getBounds() || img1.getFormat() != img2.getFormat())
        return false;

    juce::Image::BitmapData data1 (img1, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData data2 (img2, juce::Image::BitmapData::readOnly);

    for (auto y = 0; y < img1.getHeight(); ++y)
    {
        for (auto x = 0; x < img1.getWidth(); ++x)
        {
            if (img1.getFormat() == juce::Image::ARGB)
            {
                const auto pixel1 = getActualARGBPixel (data1.getPixelPointer (x, y));
                const auto pixel2 = getActualARGBPixel (data2.getPixelPointer (x, y));

                if (std::abs ((int) pixel1.a - (int) pixel2.a) > tolerance
                    || std::abs ((int) pixel1.r - (int) pixel2.r) > tolerance
                    || std::abs ((int) pixel1.g - (int) pixel2.g) > tolerance
                    || std::abs ((int) pixel1.b - (int) pixel2.b) > tolerance)
                    return false;
            }
            else
            {
                const auto pixel1 = data1.getPixelColour (x, y);
                const auto pixel2 = data2.getPixelColour (x, y);

                if (std::abs ((int) pixel1.getAlpha() - (int) pixel2.getAlpha()) > tolerance
                    || std::abs ((int) pixel1.getRed() - (int) pixel2.getRed()) > tolerance
                    || std::abs ((int) pixel1.getGreen() - (int) pixel2.getGreen()) > tolerance
                    || std::abs ((int) pixel1.getBlue() - (int) pixel2.getBlue()) > tolerance)
                    return false;
            }
        }
    }

    return true;
}

[[maybe_unused]] static int maxPixelDifference (juce::Image& img1, juce::Image& img2)
{
    if (img1.getBounds() != img2.getBounds() || img1.getFormat() != img2.getFormat())
        return 255;

    int maxDifference = 0;
    juce::Image::BitmapData data1 (img1, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData data2 (img2, juce::Image::BitmapData::readOnly);

    for (auto y = 0; y < img1.getHeight(); ++y)
    {
        for (auto x = 0; x < img1.getWidth(); ++x)
        {
            if (img1.getFormat() == juce::Image::ARGB)
            {
                const auto pixel1 = getActualARGBPixel (data1.getPixelPointer (x, y));
                const auto pixel2 = getActualARGBPixel (data2.getPixelPointer (x, y));

                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.a - (int) pixel2.a));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.r - (int) pixel2.r));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.g - (int) pixel2.g));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.b - (int) pixel2.b));
            }
            else
            {
                const auto pixel1 = data1.getPixelColour (x, y);
                const auto pixel2 = data2.getPixelColour (x, y);

                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.getAlpha() - (int) pixel2.getAlpha()));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.getRed() - (int) pixel2.getRed()));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.getGreen() - (int) pixel2.getGreen()));
                maxDifference = juce::jmax (maxDifference, std::abs ((int) pixel1.getBlue() - (int) pixel2.getBlue()));
            }
        }
    }

    return maxDifference;
}

[[maybe_unused]] static void print_test_image (juce::Image& image)
{
    // this is meant for testing trivial examples
    jassert (image.getWidth() < 20 && image.getHeight() < 20);
    std::cout << "Image: " << image.getWidth() << "x" << image.getHeight() << std::endl;

    for (auto y = 0; y < image.getHeight(); ++y)
    {
        for (auto x = 0; x < image.getWidth(); ++x)
        {
            auto color = image.getPixelAt (x, y);
            if (image.getFormat() == juce::Image::PixelFormat::SingleChannel)
                std::cout << color.getBrightness() << ", ";
            else
                std::cout << color.toDisplayString (true) << ", "; // AARRGGBB
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

[[maybe_unused]] static void save_test_image (juce::Image& image, juce::String name = "test")
{
    juce::Image imageToSave = image;
    if (imageToSave.isSingleChannel())
        imageToSave = imageToSave.convertedToFormat (juce::Image::ARGB);
#if JUCE_MAC
    auto file = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userHomeDirectory).getChildFile ("Downloads").getChildFile (name + ".png");
#else
    auto file = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile (name + ".png");
#endif
    juce::FileOutputStream stream (file);
    stream.setPosition (0);
    stream.truncate();
    juce::PNGImageFormat png;
    png.writeImageToStream (imageToSave, stream);
}
