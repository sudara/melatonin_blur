#include "../melatonin/shadows.h"
#include "helpers/pixel_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

// important for caching!
TEST_CASE ("Melatonin Blur Path Equality")
{
    SECTION ("returns true for the same path")
    {
        juce::Path path;
        path.addRectangle (1.0f, 1.0f, 1.0f, 1.0f);
        REQUIRE (melatonin::internal::CachedShadows::approximatelyEqualPaths (path, path));
    }

    SECTION ("with repeated transforms")
    {
    	juce::Path path;
        path.addRectangle (1.0f, 1.0f, 1.0f, 1.0f);

        auto translatedPath = juce::Path (path);

        for (auto i = 0; i < 10; ++i)
        {
            float offsetX = juce::Random::getSystemRandom().nextFloat();
            float offsetY = juce::Random::getSystemRandom().nextFloat();

            // move the path by a float amount
            translatedPath.applyTransform (juce::AffineTransform::translation (offsetX, offsetY));

            // move it back
            translatedPath.applyTransform (juce::AffineTransform::translation (-offsetX, -offsetY));
		}

        SECTION ("doesn't pass JUCE equality")
        {
        	REQUIRE(path != translatedPath);
        }

        SECTION ("At 1e-9, approximatelyEqualPaths returns false")
    	{
            REQUIRE (!melatonin::internal::CachedShadows::approximatelyEqualPaths (path, translatedPath, 1e-9f));
      	}

        SECTION ("At 1e-5 (default) holds up to multiple transforms")
        {
            REQUIRE (melatonin::internal::CachedShadows::approximatelyEqualPaths (path, translatedPath));
        }
    }
}
