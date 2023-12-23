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
                dropShadow.setRadius ((size_t) radiusSlider.getValue());
                innerShadow.setRadius ((size_t) radiusSlider.getValue());
                strokedDropShadow.setRadius ((size_t) radiusSlider.getValue());
                strokedInnerShadow.setRadius ((size_t) radiusSlider.getValue());
                textDropShadow.setRadius ((size_t) radiusSlider.getValue());
                textInnerShadow.setRadius ((size_t) radiusSlider.getValue());
                repaint();
            };

            spreadSlider.onValueChange = [this] {
                dropShadow.setSpread ((size_t) spreadSlider.getValue());
                innerShadow.setSpread ((size_t) spreadSlider.getValue());
                strokedDropShadow.setSpread ((size_t) spreadSlider.getValue());
                strokedInnerShadow.setSpread ((size_t) spreadSlider.getValue());
                textDropShadow.setSpread ((size_t) spreadSlider.getValue());
                textInnerShadow.setSpread ((size_t) spreadSlider.getValue());
                repaint();
            };

            offsetXSlider.onValueChange = [this] {
                dropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                innerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedDropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedInnerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                textDropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                textInnerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                repaint();
            };

            offsetYSlider.onValueChange = [this] {
                dropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                innerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedDropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedInnerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                textDropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                textInnerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                repaint();
            };

            opacitySlider.onValueChange = [this] {
                dropShadow.setOpacity ((float) opacitySlider.getValue());
                innerShadow.setOpacity ((float) opacitySlider.getValue());
                strokedDropShadow.setOpacity ((float) opacitySlider.getValue());
                strokedInnerShadow.setOpacity ((float) opacitySlider.getValue());
                textDropShadow.setOpacity ((float) opacitySlider.getValue());
                textInnerShadow.setOpacity ((float) opacitySlider.getValue());
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

            g.setColour (juce::Colours::white);
            g.setFont (juce::Font (50).boldened());
            textDropShadow.render (g, "drop", textBounds, juce::Justification::left);
            g.drawText ("drop", textBounds, juce::Justification::left);

            g.drawText ("inner", textBounds, juce::Justification::centredRight);
            textInnerShadow.render (g, "inner", textBounds.toFloat(), juce::Justification::centredRight);

            g.setFont (juce::Font (16));
            auto labels = juce::StringArray ("radius", "spread", "offsetX", "offsetY", "opacity");
            for (auto i = 0; i < labels.size(); ++i)
            {
                g.drawText (labels[i], sliderLabelsBounds.withLeft (sliderLabelsBounds.getX() + 60 * i).withWidth (60), juce::Justification::centred);
            }
            auto elapsed = juce::Time::getMillisecondCounterHiRes() - start;

            g.setColour (juce::Colours::white);
            g.setFont (juce::Font (16));
            g.drawText ("rendered in " + juce::String (elapsed, 3) + "ms", getLocalBounds().removeFromTop (50), juce::Justification::centred);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (50);
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
            animateButton.setBounds (area.removeFromTop (50).withSizeKeepingCentre (100, 30));
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
#if MELATONIN_VBLANK
        juce::VBlankAttachment vBlankCallback;
#endif
        size_t modulator = 0;
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
        juce::Font font { "Arial Black", 110.f, 0 };
        melatonin::DropShadow dropShadow {
            { juce::Colour::fromRGB (196, 181, 157), 12, { 0, 13 } },
            { juce::Colours::white, 1, { 0, -2 } }
        };
    };
}
