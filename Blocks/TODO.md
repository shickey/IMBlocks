#IMBlocks Todo List

## Basics to match existing blocks behaviors
  - Finish drag and drop -- supporting entire scripts
  - Better input processing
    - Mouse wheel to pan and zoom
    - Touch events
  - Block creation and deletion
  - Script deletion
  - Script copying (likely can use existing TearOff code)
  - Background color
  - Blocks palette
  - Categories
  - Inputs (i.e., number/string/etc on blocks)
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
    - Rotating between scripts?
    
    
    
  
## VM integration
  - Likely bring the VM code into IMBlocks
  - Pluggable runtime? What does this mean/look like? (Sketch out some usage examples)
  - Multithreading?
  

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
  