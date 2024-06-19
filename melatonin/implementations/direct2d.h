#pragma once

#include <d2d1_3.h>
#include <d3d11_3.h>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#include "juce_graphics/juce_graphics.h"
#include <juce_graphics/native/juce_Direct2DImage_windows.h>
#include <juce_graphics/native/juce_DirectX_windows.h>

namespace melatonin::blur
{
    // stdDev = radius / sqrt(2)
    constexpr float radiusToStdDev = 1.0f / 1.4142135623730951;
    static inline void direct2DSingleChannel (juce::Image& img, size_t radius)
    {
        auto sourcePixelData = dynamic_cast<juce::Direct2DPixelData*> (img.getPixelData());
        if (!sourcePixelData)
        {
            return;
        }

        if (auto& adapter = sourcePixelData->getAdapter())
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
            if ((adapter = sourcePixelData->getAdapter()))
            {
            }

            deviceContext->SetTarget (sourcePixelData->getAdapterD2D1Bitmap());
            deviceContext->BeginDraw();
            deviceContext->DrawImage (d2dEffect.get());
            deviceContext->EndDraw();
        }
    }

    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
    }
}
