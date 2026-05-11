#pragma once

#include "../blur_options.h"

// ARGB on Windows and macOS fallback when no vImage
#include "../implementations/gin.h"

// These are *compile-time* flags for implementation choices
// There are also runtime considerations
#if JUCE_MAC || JUCE_IOS

    // https://developer.apple.com/documentation/accelerate/4172615-vimagesepconvolve_argb8888
    #if (defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000) \
        || (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 170000)

        #define MELATONIN_BLUR_VIMAGE 1
        #define MELATONIN_BLUR_VIMAGE_MACOS14 1
        #include "../implementations/vImage_macOS14.h"
    #elif (defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000)
    // *Compiling* has to happen on macOS > 11.0 to support vImageSepConvolve_Planar8
    // Once compiled, we will check at runtime before relying on the vImage function
        #define MELATONIN_BLUR_VIMAGE 1
        #include "../implementations/vImage.h" // Single channel
    #else
        #include "../implementations/float_vector_stack_blur.h"
    #endif
#elif JUCE_WINDOWS
    #if MELATONIN_BLUR_USE_DIRECT2D
        #include "../implementations/direct2d.h"
    #endif

    #if defined(PAMPLEJUCE_IPP) || defined(JUCE_IPP_AVAILABLE)
        #define MELATONIN_BLUR_IPP 1
        #include "../implementations/ipp_vector.h" // single channel
    #else
        #include "../implementations/float_vector_stack_blur.h"
    #endif
#elif JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
    #include "../implementations/float_vector_stack_blur.h"
#else
  #error "Unsupported platform!"
#endif

#if JUCE_MAC || JUCE_IOS
    #include <TargetConditionals.h>
#endif

// *Runtime* checks for vImage
// Even if it compiles, we need to check when running on older devices
namespace melatonin::internal
{
    [[maybe_unused]] [[nodiscard]] static bool vImageARGBAvailable()
    {
#if defined(JUCE_MAC)
        if (__builtin_available (macOS 14.0, *))
            return true;
#elif defined(JUCE_IOS)
        if (__builtin_available (iOS 17.0, *))
            return true;
#endif
        return false;
    }

    [[maybe_unused]] [[nodiscard]] static bool vImageSingleChannelAvailable()
    {
#if defined(JUCE_MAC)
        if (__builtin_available (macOS 11.0, *))
            return true;
#elif defined(JUCE_IOS)
        if (__builtin_available (iOS 14.0, *))
            return true;
#endif
        return false;
    }
}

namespace melatonin::internal
{
    // Recreate the image only if format/bounds changed. On Windows with Direct2D
    // we disable JUCE's CPU-side backup copy: backups cost a memcpy on every
    // render and dominate tiny blurs.
    [[maybe_unused]] static inline void ensureImage (juce::Image& img,
                                                     juce::Image::PixelFormat fmt,
                                                     juce::Rectangle<int> bounds)
    {
        if (img.getFormat() == fmt && img.getBounds() == bounds)
            return;

        img = juce::Image (fmt, bounds.getWidth(), bounds.getHeight(), true);
       #if MELATONIN_BLUR_USE_DIRECT2D
        img.setBackupEnabled (false);
       #endif
    }

    // juce::Graphics short-circuits fills against an already-transparent destination,
    // so a reused image keeps the previous render's pixels. The internal context
    // bypasses that and forces an actual clear.
    [[maybe_unused]] static inline void forceClearTransparent (juce::Graphics& g,
                                                               juce::Rectangle<int> bounds)
    {
        g.getInternalContext().setFill (juce::Colours::transparentBlack);
        g.getInternalContext().fillRect (bounds, true);
    }
}

// Don't use these directly, use melatonin::CachedBlur!
namespace melatonin::blur
{
#if !MELATONIN_BLUR_VIMAGE
    [[maybe_unused]] static inline void cpuSingleChannel (juce::Image& img, size_t radius)
    {
#if defined(MELATONIN_BLUR_IPP)
        ippVectorSingleChannel (img, radius);
#else
        melatonin::blur::juceFloatVectorSingleChannel (img, radius);
#endif
    }
#endif

    [[maybe_unused]] static inline void singleChannel (juce::Image& img, size_t radius)
    {
#if MELATONIN_BLUR_VIMAGE
        if (internal::vImageSingleChannelAvailable())
            melatonin::blur::vImageSingleChannel (img, radius);
        else
            melatonin::stackBlur::ginSingleChannel (img, static_cast<unsigned int> (radius));
#elif defined(MELATONIN_BLUR_IPP)
        cpuSingleChannel (img, radius);
#else
        cpuSingleChannel (img, radius);
#endif
    }

    [[maybe_unused]] static inline void argb ([[maybe_unused]] juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
#if MELATONIN_BLUR_VIMAGE_MACOS14
        if (internal::vImageARGBAvailable())
            melatonin::blur::vImageARGB (srcImage, dstImage, radius);
        else
            melatonin::stackBlur::ginARGB (dstImage, static_cast<unsigned int> (radius));
#else
        stackBlur::ginARGB (dstImage, static_cast<unsigned int>(radius));
#endif
    }
}
