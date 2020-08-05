/*********************************************************
*
* BlocksBindings.cpp
* IMBlocks
*
* Sean Hickey
* 2020
*
* This file provides emscripten embind bindings to expose
* the blocks interface in a simple way to javascript
*
**********************************************************/

#include <emscripten/bind.h>
#include "../Blocks/Blocks.cpp"

using namespace emscripten;

// static u8 BlocksRenderInfo_vertexData_get(const BlocksRenderInfo &info) {
//   return *info.vertexData;
// }

// static void BlocksRenderInfo_vertexData_set(const BlocksRenderInfo &info, const u8 newVal) {
//   *info.vertexData = newVal;
// }

EMSCRIPTEN_BINDINGS(imblocks) {
  
  // value_array<v2>("v2")
  //   .element(&v2::x)
  //   .element(&v2::y)
  //   ;
    
  // value_array<v3>("v3")
  //   .element(&v3::x)
  //   .element(&v3::y)
  //   .element(&v3::z)
  //   ;
  
  // value_array<v4>("v4")
  //   .element(&v4::x)
  //   .element(&v4::y)
  //   .element(&v4::z)
  //   .element(&v4::w)
  //   ;
    
  // value_object<BlocksInput>("BlocksInput")
  //   .field("mouseP", &BlocksInput::mouseP)
  //   .field("mouseDown", &BlocksInput::mouseDown)
  //   .field("screenSize", &BlocksInput::screenSize)
  //   .field("wheelDelta", &BlocksInput::wheelDelta)
  //   .field("commandDown", &BlocksInput::commandDown)
  //   ;
    
  // value_object<BlocksDrawCall>("BlocksDrawCall")
  //   .field("transform", &BlocksDrawCall::transform)
  //   .field("vertexCount", &BlocksDrawCall::vertexCount)
  //   .field("vertexOffset", &BlocksDrawCall::vertexOffset)
  //   ;
    
  // value_object<BlocksRenderInfo>("BlocksRenderInfo")
  //   .field("vertexData", &BlocksRenderInfo_vertexData_get, &BlocksRenderInfo_vertexData_set)
  //   .field("vertexDataSize", &BlocksRenderInfo::vertexDataSize)
  //   .field("drawCalls", &BlocksRenderInfo::drawCalls)
  //   .field("drawCallCount", &BlocksRenderInfo::drawCallCount)
  //   ;
  
  function("InitBlocks", &InitBlocks, allow_raw_pointers());
  // function("RunBlocks", &RunBlocks, allow_raw_pointers());
  
}
