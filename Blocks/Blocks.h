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

struct BlocksRenderInfo {
    void *verts;
    u32 vertsSize;
    u32 blockVertsCount;
    u32 overlayVertsCount;
    
    mat4x4 projection;
    mat4x4 unprojection;
    
    mat4x4 overlayProjection;
};

#ifdef __cplusplus
extern "C" {
#endif
    
void InitBlocks(void *mem, u32 memSize);
BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input);

#ifdef __cplusplus
}
#endif

