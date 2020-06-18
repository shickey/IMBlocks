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
struct Script;
struct RenderBasis;

typedef u32 BlockId;
typedef u32 ScriptId;

void DrawBlock(Block *, RenderBasis *);
void DrawCommandBlock(Block *, RenderBasis *);

struct Arena {
    u8 *data;
    u32 size;
    u32 used;
};

struct Interactable {
    ScriptId id;
    f32 *x;
    f32 *y;
    
    V2 mouseOffset;
};

struct Block {
    Script *script;
    BlockType type;
    Block *next;
};

struct Script {
    ScriptId id;
    f32 x;
    f32 y;
    Block *topBlock;
};

struct BlocksContext {
    BlocksInput input;
    
    Arena verts;
    Arena uniforms;
    Arena blocks;
    u32 nextBlockIdx;
    
    Script scripts[1024];
    u32 scriptCount;
    
    Interactable hot;
    Interactable interacting;
    Interactable nextHot;
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
    Result.vertsCount = blocksCtx->verts.used / (sizeof(f32) * 4);
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

struct RenderBasis {
    f32 x;
    f32 y;
};

void RenderScript(Script *script) {
    if (!script->topBlock) { return; }
    
    RenderBasis at = { script->x, script->y };
    DrawBlock(script->topBlock, &at);
}

void DrawBlock(Block *block, RenderBasis *at) {
    switch(block->type) {
        case Command: {
            DrawCommandBlock(block, at);
            break;
        }
        default: break;
    }
}

void DrawCommandBlock(Block *block, RenderBasis *at) {
    
    BlocksRect blockRect = { at->x, at->y, 18, 16};
    if (pointInRect(blocksCtx->input.mouseP, blockRect)) {
        blocksCtx->nextHot.id = block->script->id;
        blocksCtx->nextHot.x = &block->script->x;
        blocksCtx->nextHot.y = &block->script->y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - blockRect.x, blocksCtx->input.mouseP.y - blockRect.y };
    }
    
    f32 verts[] = {
    //  X    Y                U               V
        0,   0,  (40.0 / 512.0),  (56.0 / 512.0), 
        18,  0, (472.0 / 512.0),  (56.0 / 512.0),
        0,  16,  (40.0 / 512.0), (440.0 / 512.0),
        
        18,  0, (472.0 / 512.0),  (56.0 / 512.0),
        0,  16,  (40.0 / 512.0), (440.0 / 512.0),
        18, 16, (472.0 / 512.0), (440.0 / 512.0)
    };
    
    BlockUniforms uniforms = { blocksCtx->nextBlockIdx++, at->x, at->y, false }; 
    
    pushVerts(verts);
    pushUniforms(uniforms);
    
    if (block->next) {
        at->x += 16;
        DrawBlock(block->next, at);
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
    
    context->scriptCount = 0;
    
    // Create some blocks, y'know, for fun
    Block *block1 = pushStruct(&context->blocks, Block);
    block1->type = Command;
    block1->next = NULL;
    
    Script *script1 = &context->scripts[context->scriptCount++];
    script1->id = 1;
    script1->x = -20;
    script1->y = 30;
    script1->topBlock = block1;
    
    block1->script = script1;
    
    
    
    Block *block2 = pushStruct(&context->blocks, Block);
    block2->type = Command;
    block2->next = NULL;
    
    Block *block3 = pushStruct(&context->blocks, Block);
    block3->type = Command;
    block3->next = block2;
    
    Script *script2 = &context->scripts[context->scriptCount++];
    script2->id = 2;
    script2->x = 30;
    script2->y = 40;
    script2->topBlock = block3;
    
    block2->script = script2;
    block3->script = script2;
    
    
    
    Block *block4 = pushStruct(&context->blocks, Block);
    block4->type = Command;
    block4->next = NULL;
    
    Script *script3 = &context->scripts[context->scriptCount++];
    script3->id = 3;
    script3->x = 40;
    script3->y = -30;
    script3->topBlock = block4;
    
    block4->script = script3;
}

BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input) {
    // Always reset the blocksCtx pointer in case we reloaded the dylib
    blocksCtx = (BlocksContext *)mem;
    BeginBlocks(*input);
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        Script *script = &blocksCtx->scripts[i];
        RenderScript(script);
    }
    return EndBlocks();
}

}
