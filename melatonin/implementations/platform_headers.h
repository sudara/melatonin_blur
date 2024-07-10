#pragma once

#if defined(_WIN32) || defined(_WIN64)

    #include <windows.h>
    #include <d2d1_3.h>
    #include <d2d1_3helper.h>
    #include <d3d11_3.h>
    #include <winrt/Windows.Foundation.h>

    #define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
    #include <juce_core/juce_core.h>

#endif
