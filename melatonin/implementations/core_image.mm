#define JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS 1
#include "core_image.h"
#include "../../tests/helpers/pixel_helpers.h"
#include "juce_core/juce_core.h"
#import <CoreImage/CoreImage.h>
// #include "juce_graphics/native/juce_CoreGraphicsHelpers_mac.h"
// #include "juce_graphics/native/juce_CoreGraphicsContext_mac.h"

// #include <CoreText/CTFont.h>
// #import <QuartzCore/QuartzCore.h>
// #include "juce_graphics/native/juce_CoreGraphicsContext_mac.mm"
// #include "juce_graphics/native/juce_Fonts_mac.mm"

namespace melatonin::blur
{
    // This should not happen on each call, as it sets up Metal resources needed for rendering
    // It's ok for this to be accessed from multiple threads
    // It being static here isn't the most beautiful, but should be safe
    // https://developer.apple.com/documentation/coreimage/cicontext
    // Note that the format can't be kCIFormatA8:
    // [api] CIContext working format must be kCIFormatBGRA8, kCIFormatRGBA8, kCIFormatRGBAh, kCIFormatRGBAf or nil. Ignoring request for A8.
    // Also note that this cannot be inside the function, as it will result in an EXC_BAD_ACCESS
    // https://petercompernolle.com/2015/excbadaccess-with-coreimage
    //static juce::Image contextImage = juce::Image (juce::Image::SingleChannel, 1000, 1000, true);
    static CIContext* ciContext = [CIContext contextWithOptions: nil];

    // [CIContext contextWithOptions:@{kCIContextUseSoftwareRenderer: @(YES)}] // Software renderer faster?


    void coreImageSingleChannel (juce::Image& img, size_t radius)
    {

        static auto singleColorColorSpace = juce::detail::ColorSpacePtr { CGColorSpaceCreateWithName (kCGColorSpaceGenericGrayGamma2_2) };

        //            CGDirectDisplayID displayID = CGMainDisplayID();
        //    CGSize mainScreenSize = CGDisplayScreenSize(displayID);
        //    CGRect mainScreenRect = CGDisplayBounds(displayID);

        // TODO: If we had access to the CoreGraphicsPixelData
        // could we use the cached reference for performance gains?
        CGImageRef cgImage = juce::juce_createCoreGraphicsImage (img, singleColorColorSpace.get());
        if (cgImage == nullptr)
            return;

        //        size_t width = CGImageGetWidth(cgImage);
        //        size_t height = CGImageGetHeight(cgImage);
        //        size_t bytesPerRow = CGImageGetBytesPerRow(cgImage);
        //        size_t bitsPerPixel = CGImageGetBitsPerPixel(cgImage);
        //        size_t bitsPerComponent = CGImageGetBitsPerComponent(cgImage);
        //        size_t numComponents = bitsPerPixel / bitsPerComponent;

        CIImage* inputImage = [CIImage imageWithCGImage:cgImage];

        if (!ciContext)
            jassertfalse;

        // Extend edges
        // This actually fails
        // For some reason, maybe it's related to the single channel format
        // But it results in somewhat nonsensical output such as
        //   { 0.372549027f, 0.521568656f, 0.521568656f, 0.372549027f, 0.172549024f, 0.027450981f, 0.0f, 0.0f, 0.0f, 0.0f }
        //   instead of
        //   { 0.0f, 0.0f, 0.0f, 0.247060001f, 0.74901998f, 0.74901998f, 0.247060001f, 0.0f, 0.0f, 0.0f }
        //                CIFilter* affineClampFilter = [CIFilter filterWithName:@"CIAffineClamp"];
        //                [affineClampFilter setDefaults];
        //                [affineClampFilter setValue:inputImage forKey:kCIInputImageKey];

        // Apply Gaussian blur filter
//        CIFilter* blurFilter = [CIFilter filterWithName:@ "CIBoxBlur"];
//        [blurFilter setValue:inputImage forKey:kCIInputImageKey];
//        [blurFilter setValue:@((float) radius / 3) forKey:kCIInputRadiusKey];

        //CIImage* outputImage = blurFilter.outputImage;

        // Use the input image's extent, since the outputs extent is infinite
        CGRect rect = { { 0, 0 }, { static_cast<CGFloat> (img.getWidth()), static_cast<CGFloat> (img.getHeight()) } };

        // Reposition our rect since our blur radius changed the size
        // https://developer.apple.com/documentation/coreimage/kciformatl8
        // This is where drawing from the GPU to the CPU happens.
        // So this is where all the time goes.
        CGImageRef blurredCGImage = [ciContext createCGImage:inputImage fromRect:inputImage.extent format:kCIFormatL8 colorSpace:singleColorColorSpace.get()];

       //[ciContext drawImage:outputImage  inRect:rect fromRect:rect];

        //        size_t width = CGImageGetWidth (blurredCGImage);
        //        size_t height = CGImageGetHeight (blurredCGImage);
        //        size_t bytesPerRow = CGImageGetBytesPerRow (blurredCGImage);
        //        size_t bitsPerPixel = CGImageGetBitsPerPixel (blurredCGImage);
        //        size_t bitsPerComponent = CGImageGetBitsPerComponent (blurredCGImage);
        //        size_t numComponents = bitsPerPixel / bitsPerComponent;
        //
        //        DBG ("Width: " << width);
        //        DBG ("Height: " << height);
        //        DBG ("Bytes Per Row: " << bytesPerRow);
        //        DBG ("Bits Per Pixel: " << bitsPerPixel);
        //        DBG ("Bits Per Component: " << bitsPerComponent);
        //        DBG ("Number of Components: " << numComponents);

        // Draw the blurred image back into the original CGContext
        CGContextDrawImage (juce::juce_getImageContext (img), rect, blurredCGImage);

        // save_test_image (img, "core_image_single_channel");
        CGImageRelease (cgImage);
    }

    void coreImageARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        juce::Image::BitmapData srcBitmapData (srcImage, juce::Image::BitmapData::readWrite);
        auto colorSpace = juce::detail::ColorSpacePtr { CGColorSpaceCreateWithName (kCGColorSpaceSRGB) };
        auto srcContext = juce::detail::ContextPtr { CGBitmapContextCreate (srcBitmapData.data,
            static_cast<size_t> (srcBitmapData.width),
            static_cast<size_t> (srcBitmapData.height),
            8,
            static_cast<size_t> (srcBitmapData.lineStride),
            colorSpace.get(),
            kCGImageAlphaPremultipliedLast) };

        // this is doing something strange.... without it, everything is happy
        CGImageRef cgImage = CGBitmapContextCreateImage (srcContext.get());
        CIImage* ciImage = [CIImage imageWithCGImage:cgImage];

        // Apply Gaussian blur filter
        CIFilter* blurFilter = [CIFilter filterWithName:@ "CIGaussianBlur"];
        [blurFilter setValue:ciImage forKey:kCIInputImageKey];
        [blurFilter setValue:@(radius) forKey:@"inputRadius"];
        CIImage* blurredImage = [blurFilter valueForKey:kCIOutputImageKey];

        // Render the blurred image back to the original CGContext
        CGRect extent = [blurredImage extent];
        extent.origin.x = 0;
        extent.origin.y = 0;
        CGImageRef blurredCGImage = [ciContext createCGImage:blurredImage fromRect:extent];

        juce::Image::BitmapData dstBitmapData (srcImage, juce::Image::BitmapData::readWrite);
        auto dstContext = juce::detail::ContextPtr { CGBitmapContextCreate (dstBitmapData.data,
            static_cast<size_t> (dstBitmapData.width),
            static_cast<size_t> (dstBitmapData.height),
            8,
            static_cast<size_t> (dstBitmapData.lineStride),
            colorSpace.get(),
            kCGImageAlphaPremultipliedLast) };

        // Draw the blurred image back into the original CGContext
        CGContextDrawImage (dstContext.get(), CGRectMake (0, 0, dstImage.getWidth(), dstImage.getHeight()), blurredCGImage);

        CGImageRelease (cgImage);
        CGImageRelease (blurredCGImage);
    }
} // namespace melatonin::blur
