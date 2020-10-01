# Build blocks.wasm

mkdir build
emcc -g ../../Blocks/Blocks.cpp -o build/blocks.js -s EXPORTED_FUNCTIONS='["_InitBlocks", "_RunBlocks"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "getValue", "setValue"]'
cp index.html build/index.html
cp imblocks.js build/imblocks.js
cp -r textures build/textures
