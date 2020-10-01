# IMBlocks - wasm example

This example demos the blocks library running in the browser via WebAssembly.

The `imblocks.js` file provides a very basic WebGL implementation for the blocks library to run on top of. 

## Building

Building requires emscripten and can be run via `build.sh` in this directory. This will compile the blocks library located in ../../Blocks into the necessary wasm files and drop them in the `build` directory. It will also copy index.html and imblocks.js into the build directory.

## Running

Start up a web server with `build` as the root (e.g., doing something like `python -m SimpleHTTPServer`). Point your browser to the web server url.
