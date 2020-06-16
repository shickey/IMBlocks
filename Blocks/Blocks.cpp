//
//  blocks.cpp
//  gpu-blocks-test
//
//  Created by Sean Hickey on 6/10/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#include "Blocks.h"
#include <string.h>

void CommandBlock(BlockId id, f32 x, f32 y);

struct Buffer {
    u8 *data;
    u32 size;
    u32 used;
};

struct BlocksContext {
    BlocksInput input;
    
    Buffer verts;
    Buffer uniforms;
    u32 nextBlockIdx;
    
    BlockId hot;
    BlockId interacting;
    BlockId nextHot;
};

inline
b32 pointInRect(V2 point, BlocksRect rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

global_var BlocksContext blocksCtx = { 0 };

void BeginBlocks(BlocksInput input, void *vertexMemory, u32 vertsSize, void *uniformsMemory, u32 uniformsSize) {
    blocksCtx.input = input;
    
    blocksCtx.verts.data = (u8 *)vertexMemory;
    blocksCtx.verts.size = vertsSize;
    blocksCtx.verts.used = 0;
    
    blocksCtx.uniforms.data = (u8 *)uniformsMemory;
    blocksCtx.uniforms.size = uniformsSize;
    blocksCtx.uniforms.used = 0;
    
    blocksCtx.nextBlockIdx = 0;
    
    blocksCtx.hot = 0;
    blocksCtx.nextHot = 0;
}

BlocksRenderInfo EndBlocks() {
    if (blocksCtx.interacting) {
        if (!blocksCtx.input.mouseDown) {
            blocksCtx.interacting = 0;
        }
    }
    else {
        if (blocksCtx.nextHot) {
            blocksCtx.hot = blocksCtx.nextHot;
        }
        if (blocksCtx.hot && blocksCtx.input.mouseDown) {
            blocksCtx.interacting = blocksCtx.hot;
        }
    }
    
    if (blocksCtx.hot) {
        BlockUniforms *uniforms = ((BlockUniforms *)blocksCtx.uniforms.data) + (blocksCtx.hot - 1);
        uniforms->hot = true;
    }
    
    BlocksRenderInfo Result;
    Result.verts = blocksCtx.verts.data;
    Result.vertCount = blocksCtx.verts.used / (sizeof(f32) * 2);
    Result.uniforms = blocksCtx.uniforms.data;
    Result.hot = blocksCtx.hot;
    blocksCtx.verts.data = 0;
    blocksCtx.uniforms.data = 0;
    return Result;
}

#define pushVerts(v) pushData_(&blocksCtx.verts, (v), sizeof((v)))
#define pushUniforms(u) pushData_(&blocksCtx.uniforms, &(u), sizeof((u)))
void pushData_(Buffer *buffer, void *data, u32 size) {
    Assert(buffer->used + size <= buffer->size);
    memcpy(buffer->data + buffer->used, data, size);
    buffer->used += size;
}



void Block(BlockId id, BlockType type, f32 x, f32 y) {
    Assert(blocksCtx.verts.data);
    
    switch(type) {
        case Command: {
            CommandBlock(id, x, y);
            break;
        }
        default: break;
    }
}

void CommandBlock(BlockId id, f32 x, f32 y) {
    Assert(blocksCtx.verts.data);
    
    BlocksRect blockRect = { x, y, 18, 16};
    if (pointInRect(blocksCtx.input.mouseP, blockRect)) {
        blocksCtx.nextHot = id;
    }
    
    f32 verts[] = {
    //  X   Y
    // Bottom left lobe
        0,  0,
        2,  0,
        0,  4,
        
        2,  0,
        0,  4,
        2,  4,
        
    // Left notch, lower tri
        0,  4,
        2,  4,
        2,  6,
        
    // Left notch, upper tri
        2, 10,
        2, 12,
        0, 12,
        
    // Upper left lobe
        2, 12,
        2, 16,
        0, 12,
        
        2, 16,
        0, 12,
        0, 16,
    
    // Main part
        2,  0,
        16, 0,
        2, 16,
        
        16, 0,
        2, 16,
        16, 16,
        
    // Right lobe
        16, 6,
        18, 6,
        16, 10,
        
        18, 6,
        16, 10,
        18, 10,
        
    // Right lobe, lower tri
        16, 4,
        16, 6,
        18, 6,
        
    // Right lobe, upper tri
        16, 10,
        16, 12,
        18, 10
    };
    BlockUniforms uniforms = { blocksCtx.nextBlockIdx++, x, y, false }; 
    
    pushVerts(verts);
    pushUniforms(uniforms);
    
}
