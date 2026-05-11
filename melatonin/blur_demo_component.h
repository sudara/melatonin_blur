#pragma once
#include "../melatonin_blur.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_gui_extra/juce_gui_extra.h"

#if (JUCE_MAJOR_VERSION >= 7) && (JUCE_MINOR_VERSION >= 1 || JUCE_BUILDNUMBER >= 3)
    #define MELATONIN_VBLANK 1
#else
    #define MELATONIN_VBLANK 0
#endif

namespace melatonin
{
    // TODO: Maybe someone else can make this nicer?
    // This is a little demo component you can add to your app to play with the blur
    // probably should be added at least 600x600 in size
    class BlurDemoComponent : public juce::Component, public juce::ChangeListener
    {
    public:
        BlurDemoComponent()
        {
            setOpaque (true);
            colorSelector.addChangeListener (this);

            addAndMakeVisible (radiusSlider);
            addAndMakeVisible (offsetXSlider);
            addAndMakeVisible (offsetYSlider);
            addAndMakeVisible (spreadSlider);
            addAndMakeVisible (opacitySlider);
            addAndMakeVisible (colorSelector);
            addAndMakeVisible (animateButton);
            addAndMakeVisible (resetButton);
            addAndMakeVisible (bypassCacheToggle);
            addAndMakeVisible (textShadowsToggle);

#if MELATONIN_BLUR_USE_DIRECT2D
            addAndMakeVisible (direct2DToggle);
            direct2DToggle.setToggleState (melatonin::blur::isDirect2DEnabled(), juce::dontSendNotification);
            direct2DToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
            direct2DToggle.onClick = [this] {
                melatonin::blur::setDirect2DEnabled (direct2DToggle.getToggleState());
                skipNextPerfSample = true;
                repaint();
            };
#endif

            bypassCacheToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
            bypassCacheToggle.onClick = [this] {
                const auto b = bypassCacheToggle.getToggleState();
                dropShadow.setBypassCache (b);
                innerShadow.setBypassCache (b);
                strokedDropShadow.setBypassCache (b);
                strokedInnerShadow.setBypassCache (b);
                textDropShadow.setBypassCache (b);
                textInnerShadow.setBypassCache (b);
                repaint();
            };

            textShadowsToggle.setToggleState (true, juce::dontSendNotification);
            textShadowsToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
            textShadowsToggle.onClick = [this] { repaint(); };

            radiusSlider.setColour (juce::Slider::ColourIds::trackColourId, juce::Colours::grey);
            spreadSlider.setColour (juce::Slider::ColourIds::trackColourId, juce::Colours::grey);
            offsetXSlider.setColour (juce::Slider::ColourIds::trackColourId, juce::Colours::grey);
            offsetYSlider.setColour (juce::Slider::ColourIds::trackColourId, juce::Colours::grey);
            opacitySlider.setColour (juce::Slider::ColourIds::trackColourId, juce::Colours::grey);

            radiusSlider.setColour (juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);
            spreadSlider.setColour (juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);
            offsetXSlider.setColour (juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);
            offsetYSlider.setColour (juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);
            opacitySlider.setColour (juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);

            radiusSlider.setRange (0, 100, 1);
            radiusSlider.setValue (10);
            spreadSlider.setRange (-20, 20, 1);
            offsetXSlider.setRange (-50, 50, 1);
            offsetYSlider.setRange (-50, 50, 1);
            opacitySlider.setRange (0, 1);
            opacitySlider.setNumDecimalPlacesToDisplay (2);
            opacitySlider.setValue (1);

            radiusSlider.onValueChange = [this] {
                dropShadow.setRadius (radiusSlider.getValue());
                innerShadow.setRadius (radiusSlider.getValue());
                strokedDropShadow.setRadius (radiusSlider.getValue());
                strokedInnerShadow.setRadius (radiusSlider.getValue());
                textDropShadow.setRadius (radiusSlider.getValue());
                textInnerShadow.setRadius (radiusSlider.getValue());
                repaint();
            };

            spreadSlider.onValueChange = [this] {
                dropShadow.setSpread (spreadSlider.getValue());
                innerShadow.setSpread (spreadSlider.getValue());
                strokedDropShadow.setSpread (spreadSlider.getValue());
                strokedInnerShadow.setSpread (spreadSlider.getValue());
                textDropShadow.setSpread (spreadSlider.getValue());
                textInnerShadow.setSpread (spreadSlider.getValue());
                repaint();
            };

            offsetXSlider.onValueChange = [this] {
                dropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                innerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                strokedDropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                strokedInnerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                textDropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                textInnerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                repaint();
            };

            offsetYSlider.onValueChange = [this] {
                dropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                innerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                strokedDropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                strokedInnerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                textDropShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                textInnerShadow.setOffset ({ offsetXSlider.getValue(), offsetYSlider.getValue() });
                repaint();
            };

            opacitySlider.onValueChange = [this] {
                dropShadow.setOpacity (opacitySlider.getValue());
                innerShadow.setOpacity (opacitySlider.getValue());
                strokedDropShadow.setOpacity (opacitySlider.getValue());
                strokedInnerShadow.setOpacity (opacitySlider.getValue());
                textDropShadow.setOpacity (opacitySlider.getValue());
                textInnerShadow.setOpacity (opacitySlider.getValue());
                repaint();
            };
#if MELATONIN_VBLANK
            vBlankCallback = { this,
                [this] {
                    this->repaint();
                } };
#endif
            animateButton.onClick = [this] {
                animating = !animating;
                animateButton.setButtonText (animating ? "Stop!" : "Animate!");
            };

            resetButton.onClick = [this] {
                animating = false;
                animateButton.setButtonText ("Animate!");
                modulator = 0;
                radiusSlider.setValue (10);
                spreadSlider.setValue (0);
                offsetXSlider.setValue (0);
                offsetYSlider.setValue (0);
                opacitySlider.setValue (1);
            };
        }

        void modulate()
        {
            modulator++;
            auto angle = (float) modulator / 120.0 * 2 * juce::MathConstants<float>::pi;
            auto spiralFactor = std::sin ((float) modulator / 360.0 * juce::MathConstants<float>::pi) * 0.25 + 0.5;
            auto modSin = std::sin (angle) * spiralFactor;
            auto modCos = std::cos (angle) * spiralFactor;
            auto offsetX = int (modSin * 100 / 2);
            auto offsetY = int (modCos * 100 / 2);
            offsetXSlider.setValue (offsetX, juce::sendNotificationSync);
            offsetYSlider.setValue (offsetY, juce::sendNotificationSync);
        }

        void paint (juce::Graphics& g) override
        {
            auto start = juce::Time::getMillisecondCounterHiRes();
            if (animating)
                modulate();

            g.fillAll (juce::Colours::black);
            g.setColour (contentColor);

            dropShadow.render (g, dropShadowedPath);
            g.fillPath (dropShadowedPath);

            // TODO: save/restore context!!
            g.fillPath (innerShadowedPath);
            innerShadow.render (g, innerShadowedPath);

            strokedDropShadow.render (g, strokedDropPath, juce::PathStrokeType (6));
            g.strokePath (strokedDropPath, juce::PathStrokeType (6));

            g.strokePath (strokedInnerPath, juce::PathStrokeType (6));
            strokedInnerShadow.render (g, strokedInnerPath, juce::PathStrokeType (6));

            if (textShadowsToggle.getToggleState())
            {
                g.setColour (juce::Colours::white);
                g.setFont (g.getCurrentFont().withHeight (50).boldened());
                textDropShadow.render (g, "drop", textBounds, juce::Justification::left);
                g.drawText ("drop", textBounds, juce::Justification::left);

                g.drawText ("inner", textBounds, juce::Justification::centredRight);
                textInnerShadow.render (g, "inner", textBounds.toFloat(), juce::Justification::centredRight);
            }

            g.setFont (16);
            g.setColour (juce::Colours::white);
            auto labels = juce::StringArray ("radius", "spread", "offsetX", "offsetY", "opacity");
            for (auto i = 0; i < labels.size(); ++i)
            {
                g.drawText (labels[i], sliderLabelsBounds.withLeft (sliderLabelsBounds.getX() + 60 * i).withWidth (60), juce::Justification::centred);
            }
            auto elapsed = (float) (juce::Time::getMillisecondCounterHiRes() - start);

            // After the D2D backend is flipped, the very next paint pays a one-shot
            // recalc cost across every cached shadow. Recording that single outlier
            // would shoot maxMs through the roof and squash all subsequent samples
            // into the floor of the graph, so we drop just that one frame.
            if (skipNextPerfSample)
            {
                skipNextPerfSample = false;
            }
            else
            {
                perfHistory[perfWriteIndex] = { elapsed, perfRng.nextFloat() };
                perfWriteIndex = (perfWriteIndex + 1) % kPerfHistorySize;
                if (perfFilled < kPerfHistorySize)
                    ++perfFilled;
            }

            auto topStrip = getLocalBounds().removeFromTop (80);
            auto headerArea = topStrip.removeFromTop (24);
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.setFont (13.0f);
            g.drawText ("paint durations history", headerArea, juce::Justification::centred);
            drawPerfGraph (g, topStrip);
        }

        // U+00B5 (micro sign) as explicit UTF-8 bytes. Avoids MSVC narrow-literal
        // encoding ambiguity that asserts inside juce::String when the source file
        // isn't read as UTF-8.
        static juce::String microsSuffix()
        {
            return juce::String (juce::CharPointer_UTF8 ("\xc2\xb5s"));
        }

        // scaleMs (the axis maximum) chooses the unit so every label on the axis
        // is in the same one — mixing "400µs" and "1.5ms" labels in the same graph
        // looks worse than just picking ms when the range exceeds 1ms.
        // Always one decimal place when in ms: juce::String(float, 0) means
        // "default precision" in JUCE (i.e. many digits), not "zero decimals".
        static juce::String formatTime (float ms, float scaleMs)
        {
            if (scaleMs >= 1.0f)
                return juce::String (ms, 1) + "ms";
            return juce::String (juce::roundToInt (ms * 1000.0f)) + microsSuffix();
        }

        void drawPerfGraph (juce::Graphics& g, juce::Rectangle<int> bounds)
        {
            auto labelArea = bounds.removeFromBottom (14);
            // Reserve enough horizontal space on either side of the graph for
            // the "0" / max axis labels (placed at graphArea.getX() - 40 and
            // graphArea.getRight() + 2 respectively), otherwise they get clipped.
            auto graphArea = bounds.reduced (4, 4).withTrimmedLeft (40).withTrimmedRight (62);

            float maxMs = 0.001f;
            for (size_t i = 0; i < perfFilled; ++i)
                maxMs = std::max (maxMs, perfHistory[i].elapsedMs);

            g.setFont (11.0f);
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.drawText (formatTime (0.0f, maxMs), juce::Rectangle<int> (graphArea.getX() - 40, labelArea.getY(), 38, labelArea.getHeight()), juce::Justification::centredRight);
            g.drawText (formatTime (maxMs, maxMs), juce::Rectangle<int> (graphArea.getRight() + 2, labelArea.getY(), 60, labelArea.getHeight()), juce::Justification::centredLeft);

            g.setColour (juce::Colours::white.withAlpha (0.35f));
            constexpr int numIntermediate = 5;
            for (int i = 1; i <= numIntermediate; ++i)
            {
                auto t = (float) i / (float) (numIntermediate + 1);
                auto x = juce::roundToInt (juce::jmap (t, (float) graphArea.getX(), (float) graphArea.getRight()));
                g.fillRect (x, graphArea.getBottom() - 3, 1, 3);
                g.drawText (formatTime (t * maxMs, maxMs), juce::Rectangle<int> (x - 30, labelArea.getY(), 60, labelArea.getHeight()), juce::Justification::centred);
            }

            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRect (graphArea.getX(), graphArea.getBottom() - 1, graphArea.getWidth(), 1);

            const auto liveColor = juce::Colour::fromRGB (120, 220, 180);
            const auto fadedColor = juce::Colour::fromRGB (110, 140, 125);
            const auto deadColor = juce::Colours::grey.withAlpha (0.45f);
            const auto graphTop = (float) graphArea.getY();
            const auto graphHeight = (float) graphArea.getHeight() - 4.0f;
            const auto graphX = (float) graphArea.getX();
            const auto graphWidth = (float) graphArea.getWidth();

            for (size_t i = 0; i < perfFilled; ++i)
            {
                auto idx = (perfWriteIndex + kPerfHistorySize - perfFilled + i) % kPerfHistorySize;
                const auto& s = perfHistory[idx];
                auto age = perfFilled - 1 - i;

                auto xNorm = std::min (1.0f, s.elapsedMs / maxMs);
                auto x = graphX + xNorm * graphWidth;
                auto y = graphTop + s.yJitter * graphHeight;

                g.setColour (age < 60 ? liveColor : age < 120 ? fadedColor : deadColor);
                g.fillEllipse (x - 1.5f, y - 1.5f, 3.0f, 3.0f);
            }
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (50);
            area.removeFromTop (30);
            contentBounds = area.removeFromTop (150).withSizeKeepingCentre (550, 100);
            dropShadowedPath.clear();
            dropShadowedPath.addRoundedRectangle (contentBounds.removeFromLeft (100), 10);

            contentBounds.removeFromLeft (50);
            innerShadowedPath.clear();
            innerShadowedPath.addRoundedRectangle (contentBounds.removeFromLeft (100), 10);

            contentBounds.removeFromLeft (50);
            auto strokedPathBounds = contentBounds.removeFromLeft (100);
            strokedDropPath.clear();
            strokedDropPath.addArc ((float) strokedPathBounds.getX(), (float) strokedPathBounds.getY(), (float) strokedPathBounds.getWidth(), (float) strokedPathBounds.getHeight(), 4.4f, 7.1f, true);

            contentBounds.removeFromLeft (50);
            auto strokedPathInnerBounds = contentBounds.removeFromLeft (100);
            strokedInnerPath.clear();
            strokedInnerPath.addArc ((float) strokedPathInnerBounds.getX(), (float) strokedPathInnerBounds.getY(), (float) strokedPathInnerBounds.getWidth(), (float) strokedPathInnerBounds.getHeight(), 4.4f, 7.1f, true);

            textBounds = area.removeFromTop (100).withSizeKeepingCentre (300, 100);

            auto sliderGroup = area.removeFromTop (60).withSizeKeepingCentre (300, 50);
            sliderLabelsBounds = area.removeFromTop (20).withSizeKeepingCentre (300, 20);
            radiusSlider.setBounds (sliderGroup.removeFromLeft (50));
            sliderGroup.removeFromLeft (10);
            spreadSlider.setBounds (sliderGroup.removeFromLeft (50));
            sliderGroup.removeFromLeft (10);
            offsetXSlider.setBounds (sliderGroup.removeFromLeft (50));
            sliderGroup.removeFromLeft (10);
            offsetYSlider.setBounds (sliderGroup.removeFromLeft (50));
            sliderGroup.removeFromLeft (10);
            opacitySlider.setBounds (sliderGroup.removeFromLeft (50));
            area.removeFromTop (10);

            colorSelector.setBounds (area.removeFromTop (200).withSizeKeepingCentre (200, 200));
            auto buttonRow = area.removeFromTop (50).withSizeKeepingCentre (210, 30);
            resetButton.setBounds (buttonRow.removeFromLeft (100));
            buttonRow.removeFromLeft (10);
            animateButton.setBounds (buttonRow.removeFromLeft (100));

#if MELATONIN_BLUR_USE_DIRECT2D
            auto toggleRow = area.removeFromTop (30).withSizeKeepingCentre (440, 24);
            direct2DToggle.setBounds (toggleRow.removeFromLeft (110));
            toggleRow.removeFromLeft (10);
            bypassCacheToggle.setBounds (toggleRow.removeFromLeft (140));
            toggleRow.removeFromLeft (10);
            textShadowsToggle.setBounds (toggleRow.removeFromLeft (170));
#else
            auto toggleRow = area.removeFromTop (30).withSizeKeepingCentre (320, 24);
            bypassCacheToggle.setBounds (toggleRow.removeFromLeft (140));
            toggleRow.removeFromLeft (10);
            textShadowsToggle.setBounds (toggleRow.removeFromLeft (170));
#endif
        }

        void changeListenerCallback (juce::ChangeBroadcaster* source) override
        {
            if (source == &colorSelector)
            {
                auto newColor = colorSelector.getCurrentColour().withAlpha ((float) opacitySlider.getValue());
                dropShadow.setColor (newColor);
                innerShadow.setColor (newColor);
                strokedDropShadow.setColor (newColor);
                strokedInnerShadow.setColor (newColor);
                textDropShadow.setColor (newColor);
                textInnerShadow.setColor (newColor);
                repaint();
            }
        }

    private:
        bool animating = false;
        juce::Rectangle<int> contentBounds { 0, 0, 100, 100 };
        juce::Path dropShadowedPath;
        juce::Path innerShadowedPath;
        juce::Path strokedDropPath;
        juce::Path strokedInnerPath;
        juce::Array<juce::TextButton*> typeButtons;
        juce::Colour contentColor { juce::Colours::grey };
        melatonin::DropShadow dropShadow { { juce::Colours::black, 10 } };
        melatonin::InnerShadow innerShadow { { juce::Colours::black, 10 } };
        melatonin::DropShadow strokedDropShadow { { juce::Colours::white, 10 } };
        melatonin::InnerShadow strokedInnerShadow { { juce::Colours::white, 10 } };
        melatonin::DropShadow textDropShadow { { juce::Colours::black, 10 } };
        melatonin::InnerShadow textInnerShadow { { juce::Colours::white, 2 } };
        melatonin::CachedBlur blur { 5 };
        juce::Rectangle<int> textBounds;
        juce::Rectangle<int> sliderLabelsBounds;
        juce::StringArray sliderLabels;
        juce::Slider radiusSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider spreadSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider offsetXSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider offsetYSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider opacitySlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::ColourSelector colorSelector { juce::ColourSelector::showColourAtTop
                                                 | juce::ColourSelector::editableColour
                                                 | juce::ColourSelector::showColourspace,
            0,
            0 };
        juce::TextButton animateButton { "Animate!" };
        juce::TextButton resetButton { "Reset" };
        juce::ToggleButton bypassCacheToggle { "Uncached only" };
        juce::ToggleButton textShadowsToggle { "text drop/inner" };
#if MELATONIN_BLUR_USE_DIRECT2D
        juce::ToggleButton direct2DToggle { "Direct2D" };
#endif
#if MELATONIN_VBLANK
        juce::VBlankAttachment vBlankCallback;
#endif
        size_t modulator = 0;

        struct PerfSample { float elapsedMs; float yJitter; };
        static constexpr size_t kPerfHistorySize = 300;
        std::array<PerfSample, kPerfHistorySize> perfHistory {};
        size_t perfWriteIndex = 0;
        size_t perfFilled = 0;
        bool skipNextPerfSample = false;
        juce::Random perfRng;
    };

    class TextShadowDemo : public juce::Component
    {
    public:
        void paint (juce::Graphics& g) override
        {
            // https://codepen.io/namho/pen/jEaXra
            juce::String text = "TEXTSHADOW";
            g.fillAll (juce::Colour::fromRGB (236, 229, 218));
            g.setFont (font.withExtraKerningFactor (-0.1f));

            // drop shadow renders under the text!
            dropShadow.render (g, text, getLocalBounds(), juce::Justification::centred);
            g.setColour (juce::Colour::fromRGB (241, 235, 229));
            g.drawText (text, getLocalBounds(), juce::Justification::centred);
        }

    private:
#if JUCE_MAJOR_VERSION >= 8
        juce::Font font = juce::FontOptions {}.withName ("Arial").withHeight (110.f).withStyle ("bold");
#else
        juce::Font font { "Arial", 110.f, juce::Font::bold };
#endif
        melatonin::DropShadow dropShadow {
            { juce::Colour::fromRGB (196, 181, 157), 12, { 0, 13 } },
            { juce::Colours::white, 1, { 0, -2 } }
        };
    };
}
