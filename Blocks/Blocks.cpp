//
//  blocks.cpp
//  gpu-blocks-test
//
//  Created by Sean Hickey on 6/10/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#include "Blocks.h"
#include <string.h>

struct Block;

void DrawCommandBlock(Block *);

struct Arena {
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
    
    Arena verts;
    Arena uniforms;
    Arena blocks;
    u32 numBlocks;
    u32 nextBlockIdx;
    
    Interactable hot;
    Interactable interacting;
    Interactable nextHot;
};

struct Block {
    BlockId id;
    BlockType type;
    f32 x;
    f32 y;
    Block *next;
    b32 topLevel;
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

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))
#define pushArray(arena, type, count) (type *)pushSize(arena, sizeof(type) * count)
void *pushSize(Arena *arena, u32 size) {
  // Make sure we have enough space left in the arena
  Assert(arena->used + size <= arena->size);
  
  void *result = arena->data + arena->used;
  arena->used += size;
  
  return result;
}

#define pushVerts(v) pushData_(&blocksCtx->verts, (v), sizeof((v)))
#define pushUniforms(u) pushData_(&blocksCtx->uniforms, &(u), sizeof((u)))
void pushData_(Arena *arena, void *data, u32 size) {
    void *location = pushSize(arena, size);
    memcpy(location, data, size);
}



void DrawBlock(Block *block) {
    Assert(blocksCtx->verts.data);
    
    switch(block->type) {
        case Command: {
            DrawCommandBlock(block);
            break;
        }
        default: break;
    }
}

void DrawCommandBlock(Block *block) {
    Assert(blocksCtx->verts.data);
    
    BlocksRect blockRect = { block->x, block->y, 18, 16};
    if (pointInRect(blocksCtx->input.mouseP, blockRect)) {
        blocksCtx->nextHot.id = block->id;
        blocksCtx->nextHot.x = &block->x;
        blocksCtx->nextHot.y = &block->y;
        
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
    BlockUniforms uniforms = { blocksCtx->nextBlockIdx++, block->x, block->y, false }; 
    
    pushVerts(verts);
    pushUniforms(uniforms);
    
    if (block->next) {
        block->next->x = block->x + 18;
        block->next->y = block->y;
        DrawBlock(block->next);
    }
    
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
    
    context->uniforms.data = ((u8 *)mem) + sizeof(BlocksContext) + VERTS_MEM_SIZE;
    context->uniforms.size = UNIFORMS_MEM_SIZE;
    context->uniforms.used = 0;
    
    // All the rest of the data block
    u32 blocksArenaSize = memSize - (sizeof(BlocksContext) + VERTS_MEM_SIZE + UNIFORMS_MEM_SIZE);
    context->blocks.data = ((u8 *)mem) + sizeof(BlocksContext) + VERTS_MEM_SIZE + UNIFORMS_MEM_SIZE;
    context->blocks.size = blocksArenaSize;
    context->blocks.used = 0;
    context->numBlocks = 0;
    
    // Create some blocks, y'know, for fun
    Block *block1 = pushStruct(&context->blocks, Block);
    block1->id = 1;
    block1->type = Command;
    block1->x = -20;
    block1->y = 30;
    block1->next = NULL;
    block1->topLevel = true;
    ++context->numBlocks;
    
    Block *block2 = pushStruct(&context->blocks, Block);
    block2->id = 2;
    block2->type = Command;
    block2->x = 30;
    block2->y = 40;
    block2->next = NULL;
    block2->topLevel = false;
    ++context->numBlocks;
    
    Block *block3 = pushStruct(&context->blocks, Block);
    block3->id = 3;
    block3->type = Command;
    block3->x = 0;
    block3->y = 0;
    block3->next = block2;
    block3->topLevel = true;
    ++context->numBlocks;
    
    Block *block4 = pushStruct(&context->blocks, Block);
    block4->id = 4;
    block4->type = Command;
    block4->x = 40;
    block4->y = -30;
    block4->next = NULL;
    block4->topLevel = true;
    ++context->numBlocks;
}

BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input) {
    // Always reset the blocksCtx pointer in case we reloaded the dylib
    blocksCtx = (BlocksContext *)mem;
    BeginBlocks(*input);
    for (u32 i = 0; i < blocksCtx->numBlocks; ++i) {
        Block *block = (Block *)(blocksCtx->blocks.data) + i;
        if (block->topLevel) {
            DrawBlock(block);
        }
    }
    return EndBlocks();
}

}
