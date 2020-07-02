/*********************************************************
*
* Blocks.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#include "BlocksTypes.h"

struct BlocksInput {
    union {
        struct {
            v2 mouseP;
            b32 mouseDown;
        };
        struct {
            v2 touchP;
            b32 touching;
        };
        struct {
            v2 P;
            b32 isDown;
        };
    };
    v2 screenSize;
};

struct BlocksDrawCall {
    mat4x4 transform;
    u32 vertexCount;
    u32 vertexOffset;
};

struct BlocksRenderInfo {
    u8 *vertexData;
    u32 vertexDataSize;
    
    BlocksDrawCall drawCalls[16];
    u32 drawCallCount;
};

#ifdef __cplusplus
extern "C" {
#endif
    
void InitBlocks(void *mem, u32 memSize);
BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input);

#ifdef __cplusplus
}
#endif

