#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include <d2d1_3.h>
#include <d3d11_3.h>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#include <juce_graphics/native/juce_Direct2DImage_windows.h>
#include <juce_graphics/native/juce_DirectX_windows.h>

namespace melatonin::blur
{

    struct Direct2DResources
    {
        bool valid()
        {
            return initialized && adapter && deviceContext && d2dEffect;
        }
        bool initialized = false;
        juce::DxgiAdapter::Ptr adapter;
        winrt::com_ptr<ID2D1DeviceContext2> deviceContext;
        winrt::com_ptr<ID2D1Effect> d2dEffect;
    };

    static inline Direct2DResources createResourcesIfNecessary (juce::Direct2DPixelData* sourcePixelData)
    {
        Direct2DResources resources;
        if (!sourcePixelData->getAdapter())
        {
            return resources;
        }
        if ((resources.adapter = sourcePixelData->getAdapter()))
        {
            winrt::com_ptr<ID2D1DeviceContext1> deviceContext1;
            if (const auto hr = resources.adapter->direct2DDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                    deviceContext1.put());
                FAILED (hr))
            {
                jassertfalse;
                return resources;
            }
            resources.deviceContext = deviceContext1.as<ID2D1DeviceContext2>();

            if (auto hr = resources.deviceContext->CreateEffect (CLSID_D2D1GaussianBlur, resources.d2dEffect.put()); FAILED (hr))
            {
                jassertfalse;
                return resources;
            }

            resources.initialized = true;
        }
        return resources;
    }

    static inline void direct2DSingleChannel (juce::Image& img, size_t radius)
    {
        auto sourcePixelData = dynamic_cast<juce::Direct2DPixelData*> (img.getPixelData());
        if (!sourcePixelData)
        {
            return;
        }
        auto resources = createResourcesIfNecessary (sourcePixelData);

        resources.d2dEffect->SetInput (0, sourcePixelData->getAdapterD2D1Bitmap());
        resources.deviceContext->SetTarget (sourcePixelData->getAdapterD2D1Bitmap());
        resources.deviceContext->BeginDraw();
        resources.deviceContext->DrawImage (resources.d2dEffect.get());
        resources.deviceContext->EndDraw();
    }

    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
    }
}
