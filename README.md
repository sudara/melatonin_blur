![Figma - 2023-11-09 42@2x](https://github.com/sudara/melatonin_blur/assets/472/f9e30cd7-5e2a-45fb-ab27-a353088bee31)

Melatonin Blur is a batteries-included CPU blur library for the C++ JUCE framework with a focus on performance and ease of use.

On macOS, it depends on the built-in Accelerate framework.

On Windows, it optionally depends on the Intel IPP library. If not present will fall back to a JUCE FloatVectorOperations implementation for single channel (shadows, etc) and Gin's Stack Blur for ARGB. 

## Features

*Batteries-included* means it aims to do everything you need out of the box:

* Fast! (see benchmarks below)
* Figma/CSS Accurate Drop and Inner shadows.
* Trivial to stack / layer shadows
* Behind the scenes caching of shadows and blurs (so they won't re-calculated unless their underlying data changes).
* Debug optimized (Nothing worse than painting sluggishness in Debug!)

## Installation 


### IPP on Windows

If you aren't already using it, Intel IPP might feel like an annoying dependency. Understandable! I have a blog post describing how to set it up locally and in CI. 

It's not too bad! It's fantastic tool to have for dsp as well (albeit with an annoying API!) and it'll speed up the single channel (shadows, etc) on Windows. But you'll still get Stack Blur performance and shadow caching without it.

### Via CMake and git submodules


Add the submodule:

```git
git submodule add -b main https://github.com/sudara/melatonin_blur.git modules/melatonin_blur

# To update down the road:
# git submodule update --remote --merge modules/melatonin_blur
```


or use FetchContent

```cmake
Include (FetchContent)
FetchContent_Declare (melatonin_blur
  GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
  GIT_TAG origin/main
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_blur)
FetchContent_MakeAvailable (melatonin_blur)
```

Then add this *before* your `juce_add_plugin` call:

```cmake
juce_add_module("modules/melatonin_blur")

```

Make sure to link to your plugin target *after* the `juce_add_plugin` call:

```cmake
target_link_libraries("YourProject" PRIVATE melatonin_inspector)

```

### Via Projucer

Download (via git like above, or via the UI here) and "Add a module from a specified folder" and you're done!

<p align="center">
<img src="https://github.com/sudara/melatonin_inspector/assets/472/010d9bf3-f8dc-4fc1-9039-69ba42ff856c" width="500"/>
</p>

### Don't forget to include the header!

```cpp
#include <melatonin_blur/melatonin_blur.h>
```

## Usage

### Drop Shadows

Drop shadows work on a `juce::Path`. For caching to work, `melatonin::DropShadow` needs to be a member of your `juce::Component`. In the `paint` call you will then `shadow.render()`, passing in the graphics context and the path to render. 

```cpp

class MySlider : public juce::Component
{
public:
    
    void paint (juce::Graphics& g) override
    {
        // note that drop shadows get painted *before* the path
        shadow.render (g, valueTrack);
        
        g.setColour (juce::Colours::red);
        path.fillPath (valueTrack);
    }
    
    void resized()
    {
        valueTrack.clear();
        valueTrack.addRoundedRectangle (10, 10, 100, 20, 2);
    }
    
private:
    juce::Path valueTrack;
    melatonin::DropShadow valueTrackShadow = {{ juce::Colours::black, 8, { -2, 0 } }};
}
```

The `juce::Path` doesn't *have* to be a member variable to take advantage of the shadow caching. We pass in the path to be on `render` (instead of on shadow construction) precisely to check for equality on the path before re-rendering the shadow. This lets you recalcuate the path in `paint` (for example, when `resized` isn't called), while still retaining the cached shadow (as long as the path data is the same).

```cpp
class MySlider : public juce::Component
{
public:
    
    void paint (juce::Graphics& g) override
    {
        juce::Path valueTrack;
        valueTrack.addRoundedRectangle (10, 10, 100, 20, 2);
        
        shadow.render (g, valueTrack);
        
        g.setColour (juce::Colours::red);
        path.fillPath (valueTrack);
    }
    
private:
    melatonin::DropShadow valueTrackShadow = {{ juce::Colours::black, 8, { -2, 0 } }};
}

```

Note: For paths like slider thumbs that look the same but move around, the underlying path data *will* be different as it moves, invalidating the blur cache. To avoid this, you can use a `juce::AffineTransform` to move the path around, and the cache will be retained. 


## Motivation

Blurs are essential to modern design. Layered drop shadows, frosted glass effects, blurred backgrounds — you won't see a nice looking app in the 2020s without them. 

Designers work in vector based tools such as Figma. Shadows are a big part of their workflow. With proper support for drop shadows, you can take a designer's work in Figma and quickly translate it (no need to export image strips and so on like we're in the 1990s). 

For example, I have sliders that look like this:

![Figma - 2023-11-04 47@2x](https://github.com/sudara/sinemachine/assets/472/fff21c53-5fb4-4430-8fd7-8edbc5ba0eb9)


Every single part of this slider is a vector path with shadows. The background track (2 inner), to the level indicator (3 inner, 2 drop) to the knob (3 drop shadow), is a vector path with shadows. They all need to be rendered fast enough that several of them can be happily animated at 60fps without freezing up the UI on older machines.

Stack Blur from Gin first made this UI technically possible for me. Thanks Roland! 

However, performance issues are death by 1000 cuts. I was still finding myself building little caching helpers. Sometimes these shadows added up, and I didn't feel "safe", in particular on Windows. On larger images (above 500px in a dimension) Stack Blur can takes milliseconds of CPU time (which is unacceptable for responsive UI targeting 60fps). I was also seeing some jank in Debug mode, which is discouraging.

So I started to get curious about the Stack Blur algorithm. I kept thinking I could make it:

* **Faster**. I figured a vectorized implementation would be faster than the original. The original stack blur algorithm was made for an environment without access to SIMD or intel/apple vendor libs, in 2004. For larger images, I was seeing images take `ms`. I wanted them to stay in the `µs`. In Debug, the unoptimized algorithm was sluggish and couldn't provide 60fps, even on a fast machine, so as a Bonus, this implementation is also very fast in Debug.
* **Cleaner**. The implementation in gin comes from a long line of ports starting with [Mario's old js implementation](https://web.archive.org/web/20110707030516/http://www.quasimondo.com/StackBlurForCanvas/StackBlur.js). That means there's no templates, no code reuse, there's multiplication and left shift tables to avoid division, and all kinds of trickery that's no longer needed with C++ and modern compilers. 
* **Understandable**. The concept of the "stack" in Stack Blur can take a while to "click." I had a hard time finding resources that made it easy to understand. The original notes on the algorithm don't align with how implementations semeed to work in practice. So I was interested in understanding how the algorithm works. 
* **Tested**. This is critical when iteratively re-implementing algorithms, and I felt like it was a must-have.
* **Benchmarked**. The only way to effectively compare implementations was to test on Windows and MacOS.
* **Batteries included**. 

Well, thanks to being arrogant and setting a somewhat ridiculously high bar, I spent 4-5 weeks and implemented Stack Blur probably 25 times, enough where I can do it in my sleep. There are 15 reference implementations in this repo that pass the tests (most didn't bring the performance improvements I was looking for). I still have a few more implementations that I'd like to try, but I've already invested ridiculous amounts of time into what I've been affectionately calling my C++ performance final exam, and would like to move on with my life!

## Benchmarks

Benchmarks are REALLY messy things.

* It's easy to make something *seem* like it performs 10x faster than an alternative by toying with inputs and cherry-picking results.
* It's easy to get motivated by one big number (like a 5-10x speedup on one input) and imagine that it's possible on all inputs — especially with C++ compilers, it's rarely consistently predictable.
* Initial "wow!" level improvements tend to degrade as you get closer to real world use cases (due to things like cache locality etc)
* Even with a high `n`, Benchmarks are noisy and can vary per run (musn't run other CPU intensive things while running them).
* Using benchmark averages obscure outliers (and outliers matter, especially in dsp, but even in UI).
* Results differ on different machines. 

So, here are some cherry-picked benchmarks. Here, the Windows machine is an AMD Ryzen 9 and the mac is a M1 MacBook Pro. In all cases, the image dimensions are square (e.g. 50x50px) and the times are `µs` (microseconds, or a millionth of a second) averaged over 100 runs. That means that when you see a number like 1000, it means 1ms. Please open issues if you are seeing discrepancies or want to contribute to the benchmarks. 

### Cached Drop Shadows

My #1 performance goal with this library was for drop-shadows to be screaming fast. 

99% of the time I'm rendering single channel shadows for vector UI.

Caching does most of the heavy lifting here, giving a 10-30x improvement over using just StackBlur:

<img src="https://github.com/sudara/melatonin_blur/assets/472/a17ece20-d3e4-4502-bdbc-df36f097c769" width="550" />

On Windows, with IPP as a dependency:

<img src="https://github.com/sudara/melatonin_blur/assets/472/44ee8df7-2855-4b79-a779-4e4b8b0aeb46" width="750" />

Note: I haven't been including JUCE's DropShadow class. That's in part because it's not compatible with design programs like Figma or standards like CSS, but also because it performs 20-30x worse than Stack Blur and up to 500x worse than Melatonin Blur, so the scale in `µs` has to logarithmic: 

<img src="https://github.com/sudara/melatonin_blur/assets/472/46646f22-2353-4f04-a8b5-7d1a9bc46742" width="550" />

### Single Channel Blurs (Uncached Shadows)

My #2 performance goal was for single channel blurs underlying shadows themselves to take `µs`, not `ms`. So you can also think of these as the timings for the *first* time the shadow is built. Optimizing these larger image sizes ensures that drop shadows won't be a cause for dropped frames on their first render, and can even be animated. 

Stack Blur (and in particular the Gin implementation) is already *very* optimized, especially for smaller dimensions. It's hard to beat the raw blur performance on smaller images like a 32x32px (although caching the blur is still very much worth it). However, as image dimensions scale, Stack Blur gets into the `ms`, even on single channels. 

Melatonin Blur stays under `1ms` for the initial render of most realistic image sizes and radii. 

<img src="https://github.com/sudara/melatonin_blur/assets/472/3a757c0f-d221-43f2-8a9a-ce4ae8ac8246" width="750" />

On Windows, the IPP implementation has a more consistent performance profile (when the radii changes, the timings remain about the same):

<img src="https://github.com/sudara/melatonin_blur/assets/472/9d976883-8aa9-4d41-abd1-55550dc59055" width="750" />

### Optimized for Debug Too

Debug is where we spend 95% of our day! Nothing worse than clicking around a janky low FPS UI, uncertain of how it will perform in Release. 

Because it directly talks to vendor vector libraries and the caching is still in play, Melatonin Blur is *almost* as fast in Debug as it is in Release. Individual drop shadows are up to 30x-50x faster and will stay in µs, not ms. The following chart is again on a logarithmic scale:

<img src="https://github.com/sudara/melatonin_blur/assets/472/b2b50259-2e7a-4d96-b75d-c8d508791117" width="550" />

### ARGB

The problem with ARGB blurs is it's often used to blur a whole window or a big part of the screen. The blur itself usually has to be a good 32px or something to get a good result. So the sizes are large, the radii are large, and there are 4 channels. Yikes. Bad for performance.

ARGB on Windows just about killed me. I tried many implementations, and [still have a few left to try](https://github.com/sudara/melatonin_blur/issues/2). But almost nothing outperforms Gin, so it's currently being used as the blur implementation backing Windows.

Again, the key to getting usable performance is caching. 


## FAQ

### Shouldn't this kind of thing be done on the GPU?

Yes.

### So uhh... why did you spend your life on this.

Don't ask.

### Seriously, why?

In JUCE, graphics options are limited, as it's a cross-platform C++ framework. Yes, you could spin up an OpenGL context (deprecated and crusty on MacOS), but you lose a lot of conveniences working with the normal JUCE rendering. 

Basically, JUCE got me very close to having what I needed to happily make modern plugin UIs. This library was the last piece of the puzzle.


### What's up with the tests?

The tests were necessary to verify implementations were correct and attempt to test the horizontal and vertical blur passes in isolation. Most of the implementations in this repository pass the tests.

<img src="https://github.com/sudara/sinemachine/assets/472/1e931984-6a55-4f63-8d95-a18c64e6c4f9" width="450"/>

Tests aren't currently run on this repo, there's an issue open for it.


### How can I run the benchmarks on my machine

For this first release, I put 0 effort into this. The benchmarks are there, but you'll have to feed them to an existing/another Catch2 project at the moment. There's an issue open for this.

It could be fun to codesign and release the benchmark binaries... In general I sort of daydream about JUCE hiring me to release a benchmark plugin utility for JUCE that compares dsp and UI performance across different machines (and reports to the cloud so we can all benefit from results). 

## Interesting facts learned along the way

Things I learned the hard way:

### There's no such thing as a native RGB packed image on macOS, it's all RGBA

[see this forum thread](https://forum.juce.com/t/ask-for-rgb-get-back-argb/9265).

This basically just means: under the hood, images are always 32 bytes per pixel, even if you only are using 24.

(And there's no need for an RGB blur on macOS, it's all ARGB).

### A JUCE `Image` is stored premultiplied

I ran into one gotcha when first testing the RGB(A) version of the algorithm, making sure that each channel was behaving correctly if the other channels were all at 0 and it was at 255. I couldn't get the darn tests to pass!

It turns out that JUCE, like most frameworks handling image compositing, pre-multiplies the alpha when it stores a pixel.

In other words, if the 8-bit red channel is at max, at 255, but the alpha is at a third, at 85, the red component of the pixel is actually stored pre-multiplied by the alpha value. As 85, not 255.

The alpha information is kept in the pixel, and when you ask JUCE for the color, it will unpremultiplied it so you see "red" as 255 and alpha at 85. 

This is done both for functional and performance reasons, you can [read more here](https://learn.microsoft.com/en-us/windows/apps/develop/win2d/premultiplied-alpha) and find some [interesting stuff on the JUCE forum](https://forum.juce.com/t/softwareimagerenderer-produces-premultiplied-argb/8991) about it.  

### Actual pixels are stored as `BGRA` in memory

All modern consumer MacOS and Windows machines are little endian. After banging my head for a couple hours on strangely failing tests, I realized JUCE (and most platforms it abstracts by) stores pixels in [BGRA order in memory for these machines](https://github.com/juce-framework/JUCE/blob/a7d1e61a5511875dc8f345816fca94042ce074fb/modules/juce_graphics/native/juce_CoreGraphicsContext_mac.mm#L165). 

Funny, because on the web, we think in terms of `RGBA`. 
On desktop, we think in terms of `ARGB`. 
But the computers think in `BGRA`.

### Vendor and Vector implementations

Originally I split work between "naive" (pixel by pixel) and vector implementations of Mario's Stack Blur algorithm. I learned some interesting things via the vector implementation attempts.

Image vector algos seem to need to be converted to floating point to efficiently work with vector operations on Intel and Mac. There are sometimes 8-bit functions available, but kernels have to be at least 16-bit (to calculate sums or to perform convolution). So in bulk, there's always some need for conversion. 

I tried batching the vector operations (aka, entire rows or cols of pixels) to be 32/64/100, thinking this would be a more efficient for larger images. It's not. It's slower. My best guess is vdsp/ipp under the hood juggle data and memory better than I can manually. However, the fallback implementation could still benefit from this method.

Explicit Alignment for vector implementation. Any attempts for me to do this seemed to worsen performance on Apple ARM! This is the second time I've run into this on ARM, I will have to investigate further.

It was definitely harder to get classes to perform as well as their free function counterparts. For non-vector implementations, generalizing pixel access via function pointers performs worse than using `if constexpr` in the templated function.

Vector implementations reuse vectors based on the largest dimension size. Meaning, it won't reallocate before the vertical pass. This was always faster than allocating twice, despite the vector sizes being unideal for one of the passes.

### Ugly verbose C++ code (like the original stack blur implementation) can unfortunately still outperform modern C++ code

I started out with everything in one big ole free function. 0 code reuse, as a tribute to the original.

This might seem like a strange thing to say, or even a no-brainer to a seasoned C++ developer, but I was surprised by how much of a negative impact things like templates, classes, and function calls regularly had on performance in these implementations.

In reality, I think the truth is more subtle: compilers have a lot of history with C, and are very good at optimizing a linear mess of instructions. There are more considerations when moving to templates and classes (such as memory layout, the resulting impact on cache locality, "seeing through" the indirection created by classes and function calls). The compiler can't always make the same straightforward assumptions, if only because there is more complexity for the compiler itself. My conclusion is that the human has to work *harder* at optimizing modern C++ code than with naive "risky" pointer juggling C-ish implementations. 

In other words, this project humbled me. I love to advocate for clarity, readability and maintainability. I tend to despise spaghetti code and esoteric C++ tricks. In the rare times when the absolute priority is raw speed, I still 100% believe readability and maintainability are possible, but there's definitely a price to be paid for composability and reusability. I paid that price, both in terms of raw hours, as well as an end result I wish I could be prouder of.


## Acknowledgements

* Roland Rabian for providing a JUCE Stack Blur via [Gin](https://github.com/figbug/gin).
* Luke M1 for [figuring out the `drawImageAt` optimization](https://forum.juce.com/t/faster-blur-glassmorphism-ui/43086/76). 
