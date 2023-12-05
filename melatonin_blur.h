#pragma once

/*
BEGIN_JUCE_MODULE_DECLARATION

ID:               melatonin_blur
vendor:           Sudara
version:          1.1.0
name:             Optimized CPU vector blurring and JUCE drop shadowing with tests and benchmarks
description:      Blurry Life
license:          MIT
minimumCppStandard: 17
dependencies:     juce_graphics,juce_gui_basics,juce_audio_basics

END_JUCE_MODULE_DECLARATION
*/

#include "juce_graphics/juce_graphics.h"
#include "melatonin/cached_blur.h"
#include "melatonin/shadows.h"

// These are juce::Component ImageEffects
// see https://docs.juce.com/master/classImageEffectFilter.html
#include "melatonin/image_effects/blur_effect.h"
#include "melatonin/image_effects/drop_shadow_effect.h"
#include "melatonin/image_effects/reflection_effect.h"
