#include "direct2d.h"

#if MELATONIN_BLUR_USE_DIRECT2D

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4458)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <d2d1_3.h>
 #include <d2d1effects.h>
 #include <d3d11_2.h>
 #include <dcomp.h>
 #include <dwrite_3.h>
 #include <dxgi1_3.h>
 #include <processthreadsapi.h>
JUCE_END_IGNORE_WARNINGS_MSVC

#undef min
#undef max

#include "juce_core/native/juce_ComSmartPtr_windows.h"
#include "juce_graphics/native/juce_Direct2DMetrics_windows.h"
#include "juce_graphics/native/juce_Direct2DGraphicsContext_windows.h"
#include "juce_graphics/native/juce_Direct2DPixelDataPage_windows.h"
#include "juce_graphics/images/juce_ImagePixelDataNativeExtensions.h"
#include "juce_graphics/native/juce_DirectX_windows.h"
#include "juce_graphics/native/juce_Direct2DImageContext_windows.h"

namespace melatonin::blur
{
    namespace
    {
        juce::ComSmartPtr<ID2D1Device1> getDefaultDevice()
        {
            juce::SharedResourcePointer<juce::DirectX> directX;

            if (auto adapter = directX->adapters.getDefaultAdapter())
                return adapter->direct2DDevice;

            return nullptr;
        }

        bool getSinglePage (juce::Image& image,
                            juce::ComSmartPtr<ID2D1Device1> device,
                            juce::Direct2DPixelDataPage& page)
        {
            auto pages = image.getPixelData()->getNativeExtensions().getPages (device);

            if (pages.size() != 1 || pages.front().bitmap == nullptr)
                return false;

            page = pages.front();
            return true;
        }

        bool setKernelMatrix (juce::ComSmartPtr<ID2D1Effect> effect, size_t radius, bool horizontal)
        {
            const auto kernelRadius = juce::jmax ((size_t) 1, radius);
            const auto kernelSize = 2 * kernelRadius + 1;
            const auto divisor = (float) ((kernelRadius + 1) * (kernelRadius + 1));
            std::vector<FLOAT> kernel (kernelSize);

            for (size_t i = 0; i < kernelSize; ++i)
                kernel[i] = (float) (kernelRadius + 1 - (size_t) std::abs ((int) i - (int) kernelRadius)) / divisor;

            auto hr = effect->SetValue (D2D1_CONVOLVEMATRIX_PROP_KERNEL_SIZE_X, (UINT32) (horizontal ? kernelSize : 1));
            hr |= effect->SetValue (D2D1_CONVOLVEMATRIX_PROP_KERNEL_SIZE_Y, (UINT32) (horizontal ? 1 : kernelSize));
            hr |= effect->SetValue (D2D1_CONVOLVEMATRIX_PROP_KERNEL_MATRIX,
                                    reinterpret_cast<const BYTE*> (kernel.data()),
                                    (UINT32) (kernel.size() * sizeof (FLOAT)));
            hr |= effect->SetValue (D2D1_CONVOLVEMATRIX_PROP_SCALE_MODE, D2D1_CONVOLVEMATRIX_SCALE_MODE_NEAREST_NEIGHBOR);
            return SUCCEEDED (hr);
        }

        // Owns the D2D device + device context for a blur. Both blur classes need
        // the same plumbing — this lets them share it without coupling their effects.
        struct Direct2DContext
        {
            juce::ComSmartPtr<ID2D1Device1>        device;
            juce::ComSmartPtr<ID2D1DeviceContext1> context;

            enum class State { failed, unchanged, recreated };

            // Returns recreated when the underlying device changed — callers must
            // throw away any effects they cached against the previous context.
            State ensure()
            {
                const auto defaultDevice = getDefaultDevice();
                if (defaultDevice == nullptr)
                    return State::failed;

                // Pointer equality is safe: getDefaultDevice() always returns the
                // singleton owned by juce::SharedResourcePointer<juce::DirectX>.
                if (device.get() == defaultDevice.get() && context != nullptr)
                    return State::unchanged;

                reset();
                device = defaultDevice;
                context = juce::Direct2DDeviceContext::create (device);
                return context != nullptr ? State::recreated : State::failed;
            }

            void reset()
            {
                context = {};
                device = {};
            }
        };
    }

    struct Direct2DSingleChannelBlur::Pimpl
    {
        bool render (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
        {
            if (srcImage.getPixelData() == dstImage.getPixelData())
                return false;

            if (! dstImage.setBackupEnabled (false))
                return false;

            const auto state = d2dContext.ensure();
            if (state == Direct2DContext::State::failed)
                return false;
            if (state == Direct2DContext::State::recreated)
                resetEffects();

            juce::Direct2DPixelDataPage srcPage;
            juce::Direct2DPixelDataPage dstPage;

            if (! getSinglePage (srcImage, d2dContext.device, srcPage) || ! getSinglePage (dstImage, d2dContext.device, dstPage))
                return false;

            if (! ensureEffects (radius))
                return false;

            border->SetInput (0, srcPage.bitmap);

            d2dContext.context->SetTarget (dstPage.bitmap);
            d2dContext.context->BeginDraw();
            d2dContext.context->DrawImage (vertical,
                                           nullptr,
                                           nullptr,
                                           D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                                           D2D1_COMPOSITE_MODE_SOURCE_COPY);

            const auto hr = d2dContext.context->EndDraw();
            d2dContext.context->SetTarget (nullptr);

            if (FAILED (hr))
            {
                if (hr == D2DERR_RECREATE_TARGET)
                    reset();

                return false;
            }

            return true;
        }

        void reset()
        {
            resetEffects();
            d2dContext.reset();
        }

    private:
        bool ensureEffects (size_t radius)
        {
            if (! ensureEffectGraph())
                return false;

            if (kernelConfigured && configuredRadius == radius)
                return true;

            if (! setKernelMatrix (horizontal, radius, true) || ! setKernelMatrix (vertical, radius, false))
            {
                resetEffects();
                return false;
            }

            configuredRadius = radius;
            kernelConfigured = true;
            return true;
        }

        bool ensureEffectGraph()
        {
            if (border != nullptr && horizontal != nullptr && vertical != nullptr)
                return true;

            resetEffects();

            if (const auto hr = d2dContext.context->CreateEffect (CLSID_D2D1Border, border.resetAndGetPointerAddress());
                FAILED (hr) || border == nullptr)
                return false;

            if (const auto hr = d2dContext.context->CreateEffect (CLSID_D2D1ConvolveMatrix, horizontal.resetAndGetPointerAddress());
                FAILED (hr) || horizontal == nullptr)
                return false;

            if (const auto hr = d2dContext.context->CreateEffect (CLSID_D2D1ConvolveMatrix, vertical.resetAndGetPointerAddress());
                FAILED (hr) || vertical == nullptr)
                return false;

            auto hr = border->SetValue (D2D1_BORDER_PROP_EDGE_MODE_X, D2D1_BORDER_EDGE_MODE_CLAMP);
            hr |= border->SetValue (D2D1_BORDER_PROP_EDGE_MODE_Y, D2D1_BORDER_EDGE_MODE_CLAMP);
            horizontal->SetInputEffect (0, border);
            vertical->SetInputEffect (0, horizontal);
            return SUCCEEDED (hr);
        }

        void resetEffects()
        {
            border = {};
            horizontal = {};
            vertical = {};
            configuredRadius = 0;
            kernelConfigured = false;
        }

        // Keep the D2D kitchen warm; rebuilding context/effects dominates tiny blurs.
        Direct2DContext d2dContext;
        juce::ComSmartPtr<ID2D1Effect> border;
        juce::ComSmartPtr<ID2D1Effect> horizontal;
        juce::ComSmartPtr<ID2D1Effect> vertical;
        size_t configuredRadius = 0;
        bool kernelConfigured = false;
    };

    Direct2DSingleChannelBlur::Direct2DSingleChannelBlur() : pimpl (std::make_unique<Pimpl>()) {}

    Direct2DSingleChannelBlur::~Direct2DSingleChannelBlur() = default;

    bool Direct2DSingleChannelBlur::render (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        return pimpl->render (srcImage, dstImage, radius);
    }

    void Direct2DSingleChannelBlur::reset()
    {
        pimpl->reset();
    }

    namespace
    {
        // ARGB is source-to-dest so stale destination pixels cannot get re-blurred.
        struct Direct2DARGBBlur
        {
            bool render (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
            {
                if (srcImage.getPixelData() == dstImage.getPixelData())
                    return false;

                if (! dstImage.setBackupEnabled (false))
                    return false;

                const auto state = d2dContext.ensure();
                if (state == Direct2DContext::State::failed)
                    return false;
                if (state == Direct2DContext::State::recreated)
                    resetEffect();

                juce::Direct2DPixelDataPage srcPage;
                juce::Direct2DPixelDataPage dstPage;

                if (! getSinglePage (srcImage, d2dContext.device, srcPage) || ! getSinglePage (dstImage, d2dContext.device, dstPage))
                    return false;

                if (! ensureEffect (static_cast<float> (radius) * direct2DRadiusToStdDev))
                    return false;

                effect->SetInput (0, srcPage.bitmap);

                d2dContext.context->SetTarget (dstPage.bitmap);
                d2dContext.context->BeginDraw();
                d2dContext.context->DrawImage (effect,
                                               nullptr,
                                               nullptr,
                                               D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                                               D2D1_COMPOSITE_MODE_SOURCE_COPY);

                const auto hr = d2dContext.context->EndDraw();
                d2dContext.context->SetTarget (nullptr);

                if (FAILED (hr))
                {
                    if (hr == D2DERR_RECREATE_TARGET)
                        reset();

                    return false;
                }

                return true;
            }

            void reset()
            {
                resetEffect();
                d2dContext.reset();
            }

        private:
            bool ensureEffect (float stdDev)
            {
                if (effect == nullptr)
                {
                    if (const auto hr = d2dContext.context->CreateEffect (CLSID_D2D1GaussianBlur, effect.resetAndGetPointerAddress());
                        FAILED (hr) || effect == nullptr)
                        return false;
                }

                if (stdDevConfigured && juce::approximatelyEqual (configuredStdDev, stdDev))
                    return true;

                if (FAILED (effect->SetValue (D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, stdDev)))
                {
                    effect = {};
                    stdDevConfigured = false;
                    return false;
                }

                configuredStdDev = stdDev;
                stdDevConfigured = true;
                return true;
            }

            void resetEffect()
            {
                effect = {};
                configuredStdDev = 0.0f;
                stdDevConfigured = false;
            }

            Direct2DContext d2dContext;
            juce::ComSmartPtr<ID2D1Effect> effect;
            float configuredStdDev = 0.0f;
            bool stdDevConfigured = false;
        };
    }

    bool direct2DSingleChannel (juce::Image& img, size_t radius)
    {
        jassert (img.getFormat() == juce::Image::SingleChannel);

        juce::Image blurred (juce::Image::SingleChannel, img.getWidth(), img.getHeight(), true);

        static thread_local Direct2DSingleChannelBlur blur;

        if (! blur.render (img, blurred, radius))
            return false;

        img = blurred;
        return true;
    }

    bool direct2DSingleChannel (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        jassert (srcImage.getFormat() == juce::Image::SingleChannel);
        jassert (dstImage.getFormat() == juce::Image::SingleChannel);
        jassert (srcImage.getBounds() == dstImage.getBounds());

        static thread_local Direct2DSingleChannelBlur blur;
        return blur.render (srcImage, dstImage, radius);
    }

    bool direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        jassert (srcImage.getFormat() == juce::Image::ARGB);

        if (!dstImage.isValid()
            || dstImage.getFormat() != srcImage.getFormat()
            || dstImage.getBounds() != srcImage.getBounds())
        {
            dstImage = juce::Image (srcImage.getFormat(), srcImage.getWidth(), srcImage.getHeight(), false);
        }

        static thread_local Direct2DARGBBlur blur;
        return blur.render (srcImage, dstImage, radius);
    }
}

#endif
