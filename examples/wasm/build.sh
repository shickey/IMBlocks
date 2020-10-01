# Build blocks.wasm

emcc -g ../../Blocks/Blocks.cpp -o blocks.js -s EXPORTED_FUNCTIONS='["_InitBlocks", "_RunBlocks"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "getValue", "setValue"]'