#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <d3d11_3.h>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#include "direct2d.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_graphics/native/juce_Direct2DImage_windows.h>
#include <juce_graphics/native/juce_DirectX_windows.h>
namespace melatonin::blur
{
    static inline void direct2DSingleChannel (juce::Image& img, size_t radius)
    {
        auto sourcePixelData = dynamic_cast<juce::Direct2DPixelData*> (img.getPixelData());
        if (!sourcePixelData)
        {
            return;
        }

        // we need to render to a temp image, we can't operate in-place
        auto temp = img.createCopy();
        auto tempPixelData = dynamic_cast<juce::Direct2DPixelData*> (temp.getPixelData());

        if (auto adapter = sourcePixelData->getAdapter())
        {
            winrt::com_ptr<ID2D1DeviceContext1> deviceContext1;
            if (const auto hr = adapter->direct2DDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                    deviceContext1.put());
                FAILED (hr))
            {
                jassertfalse;
            }
            auto deviceContext = deviceContext1.as<ID2D1DeviceContext2>();

            winrt::com_ptr<ID2D1Effect> d2dEffect;
            if (auto hr = deviceContext->CreateEffect (CLSID_D2D1GaussianBlur, d2dEffect.put()); FAILED (hr))
            {
                jassertfalse;
            }

            d2dEffect->SetInput (0, sourcePixelData->getAdapterD2D1Bitmap());
            d2dEffect->SetValue (D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, static_cast<float> (radius) * radiusToStdDev);
            d2dEffect->SetValue (D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
            if ((adapter = sourcePixelData->getAdapter()))
            {
            }

            deviceContext->SetTarget (tempPixelData->getAdapterD2D1Bitmap());
            deviceContext->BeginDraw();
            deviceContext->Clear();
            deviceContext->DrawImage (d2dEffect.get());
            deviceContext->EndDraw();
            deviceContext->SetTarget (nullptr);

            // copy the temp image back to the original
            img = temp;
        }
    }

    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        auto sourcePixelData = dynamic_cast<juce::Direct2DPixelData*> (srcImage.getPixelData());
        auto destPixelData = dynamic_cast<juce::Direct2DPixelData*> (dstImage.getPixelData());
        if (!sourcePixelData || !destPixelData)
        {
            return;
        }

        if (auto adapter = sourcePixelData->getAdapter())
        {
            winrt::com_ptr<ID2D1DeviceContext1> deviceContext1;
            if (const auto hr = adapter->direct2DDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                    deviceContext1.put());
                FAILED (hr))
            {
                jassertfalse;
            }
            auto deviceContext = deviceContext1.as<ID2D1DeviceContext2>();

            winrt::com_ptr<ID2D1Effect> d2dEffect;
            if (auto hr = deviceContext->CreateEffect (CLSID_D2D1GaussianBlur, d2dEffect.put()); FAILED (hr))
            {
                jassertfalse;
            }

            d2dEffect->SetInput (0, sourcePixelData->getAdapterD2D1Bitmap());
            d2dEffect->SetValue (D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, static_cast<float> (radius) * radiusToStdDev);
            d2dEffect->SetValue (D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
            if ((adapter = sourcePixelData->getAdapter()))
            {
            }

            deviceContext->SetTarget (destPixelData->getAdapterD2D1Bitmap());
            deviceContext->BeginDraw();
            deviceContext->Clear();
            deviceContext->DrawImage (d2dEffect.get());
            deviceContext->EndDraw();
            deviceContext->SetTarget (nullptr);
        }
    }
}
