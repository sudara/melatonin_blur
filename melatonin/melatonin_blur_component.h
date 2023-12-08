#pragma once
#include "../melatonin_blur.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_gui_extra/juce_gui_extra.h"

namespace melatonin
{
    // TODO: Maybe someone else can make this nicer?
    // This is a little demo component you can add to your app to play with the blur
    // probably should be added at least 200, 500 in size
    class BlurDemoComponent : public juce::Component, public juce::ChangeListener
    {
    public:
        BlurDemoComponent()
        {
            colorSelector.addChangeListener (this);

            addAndMakeVisible (radiusSlider);
            addAndMakeVisible (offsetXSlider);
            addAndMakeVisible (offsetYSlider);
            addAndMakeVisible (spreadSlider);
            addAndMakeVisible (opacitySlider);
            addAndMakeVisible (colorSelector);

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
                strokedPathShadow.setRadius ((size_t) radiusSlider.getValue());
                repaint();
            };

            spreadSlider.onValueChange = [this] {
                dropShadow.setSpread ((size_t) spreadSlider.getValue());
                innerShadow.setSpread ((size_t) spreadSlider.getValue());
                strokedPathShadow.setSpread ((size_t) spreadSlider.getValue());
                repaint();
            };

            offsetXSlider.onValueChange = [this] {
                dropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                innerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedPathShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                repaint();
            };

            offsetYSlider.onValueChange = [this] {
                dropShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                innerShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                strokedPathShadow.setOffset ({ (int) offsetXSlider.getValue(), (int) offsetYSlider.getValue() });
                repaint();
            };

            opacitySlider.onValueChange = [this] {
                dropShadow.setOpacity ((float) opacitySlider.getValue());
                innerShadow.setOpacity ((float) opacitySlider.getValue());
                strokedPathShadow.setOpacity ((float) opacitySlider.getValue());
                repaint();
            };
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colours::black);
            g.setColour (contentColor);
            dropShadow.render (g, dropShadowedPath);
            g.fillPath (dropShadowedPath);

            // TODO: save/restore context!!
            g.fillPath (innerShadowedPath);
            innerShadow.render (g, innerShadowedPath);

            auto labels = juce::StringArray ("radius", "spread", "offsetX", "offsetY", "opacity");
            for (auto i = 0; i < labels.size(); ++i)
            {
                g.drawText (labels[i], sliderLabelsBounds.withLeft (sliderLabelsBounds.getX() + 60 * i).withWidth (60), juce::Justification::centred);
            }

            strokedPathShadow.renderStroked (g, strokedPath, juce::PathStrokeType (6));
            g.strokePath (strokedPath, juce::PathStrokeType (6));
            // g.drawImageAt (blur.render(), contentBounds.getX(), contentBounds.getY());
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (50);
            contentBounds = area.removeFromTop (200).withSizeKeepingCentre (500, 100);
            dropShadowedPath.clear();
            dropShadowedPath.addRoundedRectangle (contentBounds.removeFromLeft (100), 10);

            contentBounds.removeFromLeft (100);
            innerShadowedPath.clear();
            innerShadowedPath.addRoundedRectangle (contentBounds.removeFromLeft (100), 10);

            contentBounds.removeFromLeft (100);
            auto strokedPathBounds = contentBounds.removeFromLeft (100);
            strokedPath.clear();
            strokedPath.addArc ((float) strokedPathBounds.getX(), (float) strokedPathBounds.getY(), (float) strokedPathBounds.getWidth(), (float) strokedPathBounds.getHeight(), 4.4f, 7.1f, true);

            // strokedPath.clear();
            // strokedPath.addArc (contentBounds.getX(), contentBounds.getY(), contentBounds.getWidth(), contentBounds.getHeight(), 4.5, 7.9, true);

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
        }

        void changeListenerCallback (juce::ChangeBroadcaster* source) override
        {
            if (source == &colorSelector)
            {
                dropShadow.setColor (colorSelector.getCurrentColour());
                innerShadow.setColor (colorSelector.getCurrentColour());
                strokedPathShadow.setColor (colorSelector.getCurrentColour());
                repaint();
            }
        }

    private:
        juce::Rectangle<int> contentBounds { 0, 0, 100, 100 };
        juce::Path dropShadowedPath;
        juce::Path innerShadowedPath;
        juce::Path strokedPath;
        juce::Array<juce::TextButton*> typeButtons;
        juce::Colour contentColor { juce::Colours::grey };
        melatonin::DropShadow dropShadow { { juce::Colours::black, 10 } };
        melatonin::DropShadow strokedPathShadow { { juce::Colours::black, 10 } };
        melatonin::InnerShadow innerShadow { { juce::Colours::white, 10 } };
        melatonin::CachedBlur blur { 5 };
        juce::Rectangle<int> sliderLabelsBounds;
        juce::StringArray sliderLabels;
        juce::Slider radiusSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider spreadSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider offsetXSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider offsetYSlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::Slider opacitySlider { juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow };
        juce::ColourSelector colorSelector { juce::ColourSelector::showColourAtTop
                                                 | juce::ColourSelector::editableColour
                                                 | juce::ColourSelector::showSliders
                                                 | juce::ColourSelector::showColourspace,
            0,
            0 };
    };
}
