#pragma once

namespace melatonin::blur
{
    // stdDev = radius / sqrt(2)
    constexpr float radiusToStdDev = 1.0f / 1.4142135623730951f;
    static inline void direct2DSingleChannel ([[maybe_unused]] juce::Image& img, [[maybe_unused]] size_t radius)
    {
#if 0
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
#endif
    }

    static inline void direct2DARGB (juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
        mescal::Effect effect { mescal::Effect::Type::gaussianBlur };

        effect.setProperty(mescal::Effect::GaussianBlurPropertyIndex::blurAmount, (float)radius * radiusToStdDev);
        effect.applyEffect (srcImage, dstImage, 1.0f, 1.0f);
    }

    static inline void direct2DARGB(juce::Image& image, size_t radius)
    {
        juce::Image temp { juce::Image::ARGB, image.getWidth(), image.getHeight(), true };

        direct2DARGB (image, temp, radius);
        image = temp;
    }
}
