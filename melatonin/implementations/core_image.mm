#include "core_image.h"
#import <CoreImage/CoreImage.h>

namespace melatonin::blur
{
    void coreImageSingleChannel (juce::Image& img, size_t radius)
    {
//        juce::Image::BitmapData bitmapData (img, juce::Image::BitmapData::readWrite);
        auto colorSpace = juce::detail::ColorSpacePtr { CGColorSpaceCreateWithName (kCGColorSpaceGenericGrayGamma2_2) };
//        auto context = juce::detail::ContextPtr { CGBitmapContextCreate (bitmapData.data,
//            static_cast<size_t> (bitmapData.width),
//            static_cast<size_t> (bitmapData.height),
//            8,
//            static_cast<size_t> (bitmapData.lineStride),
//            colorSpace.get(),
//            kCGImageAlphaPremultipliedLast) };
//
//        CGImageRef cgImage = CGBitmapContextCreateImage (context.get());
//
            // Convert JUCE Image to CGImageRef
        CGImageRef cgImage = juce::juce_createCoreGraphicsImage (img, colorSpace.get());
        if (cgImage == nullptr)
            return;

        CIImage* ciImage = [CIImage imageWithCGImage:cgImage];
        CIContext* ciContext = [CIContext contextWithOptions:nil];

        // Extend edges
        CIFilter *affineClampFilter = [CIFilter filterWithName:@"CIAffineClamp"];
        [affineClampFilter setDefaults];
        [affineClampFilter setValue:ciImage forKey:kCIInputImageKey];

        // Apply Gaussian blur filter
        CIFilter* blurFilter = [CIFilter filterWithName:@ "CIGaussianBlur"];
        [blurFilter setValue:ciImage forKey:kCIInputImageKey];
        [blurFilter setValue:@(radius) forKey:kCIInputRadiusKey];
        CIImage* blurredImage = blurFilter.outputImage;

        // Render the blurred image back to the original CGContext
        CGRect extent = [blurredImage extent];
        CGImageRef blurredCGImage = [ciContext createCGImage:blurredImage fromRect:extent];

        // Draw the blurred image back into the original CGContext
        CGContextDrawImage (juce::juce_getImageContext(img), CGRectMake (0, 0, img.getWidth(), img.getHeight()), blurredCGImage);

        CGImageRelease (cgImage);
        CGImageRelease (blurredCGImage);
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

        CGImageRef cgImage = CGBitmapContextCreateImage (srcContext.get());
        CIImage* ciImage = [CIImage imageWithCGImage:cgImage];
        CIContext* ciContext = [CIContext contextWithOptions:nil];

        // Apply Gaussian blur filter
        CIFilter* blurFilter = [CIFilter filterWithName:@ "CIGaussianBlur"];
        [blurFilter setValue:ciImage forKey:kCIInputImageKey];
        [blurFilter setValue:@(radius) forKey:@"inputRadius"];
        CIImage* blurredImage = [blurFilter valueForKey:kCIOutputImageKey];

        // Render the blurred image back to the original CGContext
        CGRect extent = [blurredImage extent];
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
