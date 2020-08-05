#IMBlocks Todo List

## Basics to match existing blocks behaviors
  - Input system for fields
  - Bug: text rendering on top of everything else (need to combine sdfs into single texture (or z buffer or something))
  - Better input processing
    - Touch events
  - Block creation and deletion
  - Script copying (likely can use existing TearOff code)
  - Blocks palette
  - Categories
  - Renderer
    - Block texturing (icons, etc)
    - Mipmapping for small block sizes, (keep SDF for larger)
  - Memory Management
    - Custom allocators?
    - Arena growing
    - General purpose malloc-style sub-allocator
  - Animations
    - Discarding scripts
    - Scrolling palette
    - Zoom in on focused input
    - Rotating between scripts?
  - Serialization
  - Zoom and pan: add boundaries
    
### Done?
  - Finish drag and drop
    - Supporting entire scripts
    - Prioritizing connection types
    - Connecting and combining scripts on drop
  - Script deletion
  - Mouse wheel to pan and zoom
  - Background color
  - Inputs (i.e., number/string/etc on blocks)
  - Input fields
  - Debug System
    - Simple string rendering
  
## VM integration
  - Likely bring the VM code into IMBlocks
  - Pluggable runtime? What does this mean/look like? (Sketch out some usage examples)
  - Multithreading?
  - "Compile" to bytecode which is then consumed by the VM


## Example Projects
  - Mac
  - iOS
  - Android
  - wasm

## Stuff to explore
  - Horizontal grammar
    - Branching?
    - Rotate between horizontal and vertical grammars
  - Mini vertical grammar? (e.g., icons only)
  - Shrinkable scripts
  - One script at a time, rotate between them


## Eventually
  - Design iterations on API
  - Remote API? Using blocks on phone to program on computer, projector, another device, etc.?
