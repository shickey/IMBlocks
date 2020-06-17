//
//  blocks.cpp
//  gpu-blocks-test
//
//  Created by Sean Hickey on 6/10/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#include "Blocks.h"
#include <string.h>

void CommandBlock(BlockId id, f32 *x, f32 *y);

struct Buffer {
    u8 *data;
    u32 size;
    u32 used;
};

struct Interactable {
    BlockId id;
    f32 *x;
    f32 *y;
    
    V2 mouseOffset;
};

struct BlocksContext {
    BlocksInput input;
    
    Buffer verts;
    Buffer uniforms;
    u32 nextBlockIdx;
    
    Interactable hot;
    Interactable interacting;
    Interactable nextHot;
};

struct BlockData {
    f32 x;
    f32 y;
};

static BlockData blockData[4] = {
    {-40, 30},
    {0, 60},
    {20, -30},
    {-40, -30}
};

inline
b32 pointInRect(V2 point, BlocksRect rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

global_var BlocksContext *blocksCtx = 0;

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear memory
    blocksCtx->verts.used = 0;
    blocksCtx->uniforms.used = 0;
    
    blocksCtx->nextBlockIdx = 0;
    
    blocksCtx->hot.id = 0;
    blocksCtx->nextHot.id = 0;
}

BlocksRenderInfo EndBlocks() {
    if (blocksCtx->interacting.id) {
        if (!blocksCtx->input.mouseDown) {
            blocksCtx->interacting.id = 0;
        }
        else {
            *blocksCtx->interacting.x = blocksCtx->input.mouseP.x - blocksCtx->interacting.mouseOffset.x;
            *blocksCtx->interacting.y = blocksCtx->input.mouseP.y - blocksCtx->interacting.mouseOffset.y;
        }
    }
    else {
        if (blocksCtx->nextHot.id) {
            blocksCtx->hot = blocksCtx->nextHot;
        }
        if (blocksCtx->hot.id && blocksCtx->input.mouseDown) {
            blocksCtx->interacting = blocksCtx->hot;
        }
    }
    
    if (blocksCtx->hot.id) {
        BlockUniforms *uniforms = ((BlockUniforms *)blocksCtx->uniforms.data) + (blocksCtx->hot.id - 1);
        uniforms->hot = true;
    }
    
    BlocksRenderInfo Result;
    Result.verts = blocksCtx->verts.data;
    Result.vertsCount = blocksCtx->verts.used / (sizeof(f32) * 2);
    Result.vertsSize = blocksCtx->verts.used;
    Result.uniforms = blocksCtx->uniforms.data;
    Result.uniformsSize = blocksCtx->uniforms.used; 
    return Result;
}

#define pushVerts(v) pushData_(&blocksCtx->verts, (v), sizeof((v)))
#define pushUniforms(u) pushData_(&blocksCtx->uniforms, &(u), sizeof((u)))
void pushData_(Buffer *buffer, void *data, u32 size) {
    Assert(buffer->used + size <= buffer->size);
    memcpy(buffer->data + buffer->used, data, size);
    buffer->used += size;
}



void Block(BlockId id, BlockType type, f32 *x, f32 *y) {
    Assert(blocksCtx->verts.data);
    
    switch(type) {
        case Command: {
            CommandBlock(id, x, y);
            break;
        }
        default: break;
    }
}

void CommandBlock(BlockId id, f32 *x, f32 *y) {
    Assert(blocksCtx->verts.data);
    
    BlocksRect blockRect = { *x, *y, 18, 16};
    if (pointInRect(blocksCtx->input.mouseP, blockRect)) {
        blocksCtx->nextHot.id = id;
        blocksCtx->nextHot.x = x;
        blocksCtx->nextHot.y = y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - blockRect.x, blocksCtx->input.mouseP.y - blockRect.y };
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
    BlockUniforms uniforms = { blocksCtx->nextBlockIdx++, *x, *y, false }; 
    
    pushVerts(verts);
    pushUniforms(uniforms);
    
}

extern "C" {

void InitBlocks(void *mem, u32 memSize) {
    static const u32 VERTS_MEM_SIZE = 65535 * (2 * sizeof(f32));
    static const u32 UNIFORMS_MEM_SIZE = 4096 * sizeof(BlockUniforms);
    
    Assert(memSize >= sizeof(BlocksContext) + VERTS_MEM_SIZE + UNIFORMS_MEM_SIZE);
    
    BlocksContext *context = (BlocksContext *)mem;
    
    context->verts.data = ((u8 *)mem) + sizeof(BlocksContext);
    context->verts.size = VERTS_MEM_SIZE;
    context->verts.used = 0;
    
    context->uniforms.data = ((u8 *)mem) + VERTS_MEM_SIZE + sizeof(BlocksContext);
    context->uniforms.size = UNIFORMS_MEM_SIZE;
    context->uniforms.used = 0;
}

BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input) {
    // Always reset the blocksCtx pointer in case we reloaded the dylib
    blocksCtx = (BlocksContext *)mem;
    BeginBlocks(*input);
    for (u32 i = 0; i < 3; ++i) {
        BlockData *data = &blockData[i];
        Block(i + 1, Command, &data->x, &data->y);
    }
    return EndBlocks();
}

}
