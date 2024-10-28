![Figma - 2023-11-09 42@2x](https://github.com/sudara/melatonin_blur/assets/472/0cb16190-bce7-4d9a-8a7c-d15846946354)

![](https://github.com/sudara/melatonin_blur/actions/workflows/tests.yml/badge.svg)

Melatonin Blur is a batteries-included, cross-platform CPU blur and shadow compositing library for
the [JUCE C++ framework](https://juce.com/).

*Batteries-included* means it aims to give you everything out of the box:

* 👩‍🎨 Figma/CSS-accurate drop and inner shadows on paths
* 🔠 Drop and Inner Text shadows
* 💅🏼 Supports both filled and stroked paths
* 🌇 ARGB image blurs
* 🚀 Fast! (see [benchmarks](#more-benchmarks))
* 🔎 Retina-friendly (context scale-aware)
* 🍰 Trivial to layer multiple shadows
* ⚙️ Behind-the-scenes multi-layer caching
* 😎 Debug optimized for high quality of life
* 🤖 Over 1000 correctness tests passing on macOS/windows
* 🚂 Compatible down to macOS 10.13 (progressive speedups on recent versions)

The goal: modern vector interfaces in JUCE (100s of shadows) without having to resort to deprecated solutions with lower quality of life (looking at you, OpenGL on macOS!).

https://github.com/sudara/melatonin_blur/assets/472/3e1d6c9a-aab9-422f-a262-6b63cbca5b71

Melatonin Blur provides a 10-30x speedup over using Stack Blur alone.

<img src="https://github.com/sudara/melatonin_blur/assets/472/598115b8-9c9d-42d8-b868-e921978cda17" width="550" />

On macOS, it depends on the built-in Accelerate framework.

On Windows, it *optionally* depends on the Intel IPP library. If IPP is not present, it will fall back to a JUCE FloatVectorOperations implementation for single channel (shadows, etc) and Gin's Stack Blur for ARGB.

Interested in how the blurring
works? [I wrote an in-depth article about re-implementing Stack Blur 15+ times](https://melatonin.dev/blog/implementing-marios-stack-blur-15-times-in-cpp/).

## Installation

The docs have outgrown a README file, [visit the new documentation](https://melatonin.dev/manuals/melatonin-blur/installation/cmake/)

[<div align="center"><img src="https://github.com/user-attachments/assets/8eb5a814-1457-4fd0-a979-e9c1cfd800cb" width="450"/></div>](https://melatonin.dev/manuals/melatonin-blur/installation/cmake/)

## Usage

There's all the usage information you could want over at [the official docs](https://melatonin.dev/manuals/melatonin-blur/usage/drop-shadows/)

[<div align="center"><img src="https://github.com/user-attachments/assets/3abedb03-4c15-4c3e-b04e-f7d36eb144aa" width="450"/></div>](https://melatonin.dev/manuals/melatonin-blur/usage/drop-shadows/)


## Acknowledgements

* Mars, for being my reliable rubber duck! Inner Shadow caching and compositing geometry broke my brain.
* Roland Rabien for JUCE Stack Blur workhorse via [Gin](https://github.com/figbug/gin).
* LukeM1 on the forums
  for [figuring out the `drawImageAt` optimization](https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76).
* Ecstasy on the Discord for the motivation and feedback around stroked paths and default constructors.
