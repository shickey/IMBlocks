# IMBlocks

A library for running Scratch Blocks-esque programming environments on GPU hardware.

NOTE: Extremely unfinished. More of a proof-of-concept than anything.

This project aims to be as lightweight and platform agnostic as possible. Some key things to know in regard to this:

- Zero dependencies. And standards-compliant C/C++ compiler should be able to compile this.
- Zero memory allocation. Your app allocates a big chunk of memory up front and passes it to IMBlocks which manages its own memory (i.e., no `new`, no `malloc`, no `free`, etc.)
- GPU backend agnostic. Call one function each time you want to draw the blocks (e.g., once per frame tick), get back vertex data that can be drawn with any API that can render textured triangles (e.g., OpenGL, Metal, D3D, WebGL, Vulkan, whatever).
- "Immediate mode" paradigm code style. No data to retain across draw calls. (Look up IMGUI paradigm for further explanation).

# Usage

Include `Blocks.h` into your renderer.

``` c
#include "Blocks.h"
```

Allocate a big chunk of memory and pass it to the IMBlocks initialize to get started. This memory will be used for the lifetime of your app.

``` c
uint32_t memSize = 128 * 1024 * 1024; // 128 mb
void *blocksMem = malloc(memSize);    // Allocate your blocks memory

InitBlocks(blocksMem, memSize);       // Initialize IMBlocks with your blocks memory
```

Each time you want to update and/or draw your blocks interface, create an input structure (which holds info like key presses and mouse events). Pass the input structure to IMBlocks. Receive back a structure with vertex data for drawing using your GPU backend.

``` c

BlocksInput blocksInput; // Create a new BlocksInput struct

// Fill out the structure
blocksInput.mouseP = ...      // Current mouse position
blocksInput.mouseDown = ...   // Is the mouse button pressed right now?
blocksInput.wheelDelta = ...  // Amount the mouse wheel has moved since last frame
blocksInput.commandDown = ... // Is the command button down on the keyboard?
blocksInput.screenSize = ...  // Current rendering size (can account for high DPI here)

// Update IMBlocks
BlocksRenderInfo renderInfo = RunBlocks(blocksMem, &blocksInput);

// Draw vertex data from IMBlocks
for (uint32_t i = 0; i < renderInfo.drawCallCount; ++i) {
    BlocksDrawCall *drawCall = &renderInfo.drawCalls[i];
    ...
    // Draw
}
```

# Examples

The examples directory contains a few different examples for using the library. The Mac/iOS example uses Metal as the rendering backend. The wasm example uses WebGL and runs in the browser. See the README in each example directory for more info on each.
