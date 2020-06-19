/*********************************************************
*
* Blocks.cpp
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#include "Blocks.h"
#include "BlocksInternal.h"
#include "BlocksVerts.h"

global_var BlocksContext *blocksCtx = 0;

inline
b32 pointInRect(v2 point, BlocksRect rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear memory
    blocksCtx->verts.used = 0;
    
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
    
    BlocksRenderInfo Result;
    Result.verts = blocksCtx->verts.data;
    Result.vertsCount = blocksCtx->verts.used / (sizeof(f32) * 7);
    Result.vertsSize = blocksCtx->verts.used;
    return Result;
}

void RenderScript(Script *script) {
    if (!script->topBlock) { return; }
    
    RenderBasis basis = { script->x, script->y, 0, 0 };
    DrawBlock(script->topBlock, &basis);
}

void DrawBlock(Block *block, RenderBasis *basis) {
    switch(block->type) {
        case Command: {
            DrawCommandBlock(block, basis);
            break;
        }
        case Loop: {
            DrawLoopBlock(block, basis);
        }
        default: break;
    }
}

void DrawCommandBlock(Block *block, RenderBasis *basis) {
    
    BlocksRect hitBox = { basis->at.x, basis->at.y, 16, 16};
    
    PushCommandBlockVerts(blocksCtx, v2{basis->at.x, basis->at.y}, v3{0, 1, 1});
    
    if (block->next) {
        basis->at.x += 16;
        DrawBlock(block->next, basis);
    }
    
    basis->bounds.x += 16;
    basis->bounds.y = Max(basis->bounds.y, 16);
    
    if (pointInRect(blocksCtx->input.mouseP, hitBox)) {
        blocksCtx->nextHot.id = block->script->id;
        blocksCtx->nextHot.x = &block->script->x;
        blocksCtx->nextHot.y = &block->script->y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - block->script->x, blocksCtx->input.mouseP.y - block->script->y };
    }
    
}

void DrawLoopBlock(Block *block, RenderBasis *basis) {
    
    u32 horizStretch = 0;
    u32 vertStretch = 0;
    if (block->inner) {
        RenderBasis innerBasis = { basis->at.x + 6, basis->at.y, 0, 0 };
        DrawBlock(block->inner, &innerBasis);
        horizStretch = (u32)(innerBasis.bounds.w - 16);
        vertStretch = (u32)(innerBasis.bounds.h - 16);
    }
    
    BlocksRect hitBox = { basis->at.x, basis->at.y, 38 + (f32)horizStretch, 20 + (f32)vertStretch };
    
    PushLoopBlockVerts(blocksCtx, v2{basis->at.x, basis->at.y}, v3{1, 0, 1}, horizStretch, vertStretch);
    
    if (block->next) {
        basis->at.x += 38 + horizStretch;
        DrawBlock(block->next, basis);
    }
    
    basis->bounds.x += 38 + horizStretch;
    basis->bounds.y = Max(basis->bounds.y, 20 + vertStretch);
    
    if (pointInRect(blocksCtx->input.mouseP, hitBox)) {
        blocksCtx->nextHot.id = block->script->id;
        blocksCtx->nextHot.x = &block->script->x;
        blocksCtx->nextHot.y = &block->script->y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - block->script->x, blocksCtx->input.mouseP.y - block->script->y };
    }
    
}


extern "C" void InitBlocks(void *mem, u32 memSize) {
    static const u32 VERTS_MEM_SIZE = 65535 * (2 * sizeof(f32));
    
    Assert(memSize >= sizeof(BlocksContext) + VERTS_MEM_SIZE);
    
    BlocksContext *context = (BlocksContext *)mem;
    
    context->verts.data = ((u8 *)mem) + sizeof(BlocksContext);
    context->verts.size = VERTS_MEM_SIZE;
    context->verts.used = 0;
    
    // All the rest of the data block
    u32 blocksArenaSize = memSize - (sizeof(BlocksContext) + VERTS_MEM_SIZE);
    context->blocks.data = ((u8 *)mem) + sizeof(BlocksContext) + VERTS_MEM_SIZE;
    context->blocks.size = blocksArenaSize;
    context->blocks.used = 0;
    
    context->scriptCount = 0;
    
    // Create some blocks, y'know, for fun
    Block *block1 = pushStruct(&context->blocks, Block);
    block1->type = Command;
    block1->next = NULL;
    
    Script *script1 = &context->scripts[context->scriptCount++];
    script1->id = 1;
    script1->x = 0;
    script1->y = 0;
    script1->topBlock = block1;
    
    block1->script = script1;
    
    
    
    Block *block2 = pushStruct(&context->blocks, Block);
    block2->type = Command;
    block2->next = NULL;
    
    Block *loop = pushStruct(&context->blocks, Block);
    loop->type = Loop;
    loop->next = block2;
    
    Block *block3 = pushStruct(&context->blocks, Block);
    block3->type = Command;
    block3->next = loop;
    
    Script *script2 = &context->scripts[context->scriptCount++];
    script2->id = 2;
    script2->x = -20;
    script2->y = 40;
    script2->topBlock = block3;
    
    block2->script = script2;
    loop->script = script2;
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
    
    
    
    Block *block5 = pushStruct(&context->blocks, Block);
    block5->type = Command;
    block5->next = NULL;
    
    Block *block6 = pushStruct(&context->blocks, Block);
    block6->type = Command;
    block6->next = NULL;
    
    Block *block7 = pushStruct(&context->blocks, Block);
    block7->type = Command;
    block7->next = block6;
    
    Block *loop2 = pushStruct(&context->blocks, Block);
    loop2->type = Loop;
    loop2->next = block5;
    loop2->inner = block7;
    
    Block *block8 = pushStruct(&context->blocks, Block);
    block8->type = Command;
    block8->next = loop2;
    
    Block *loop3 = pushStruct(&context->blocks, Block);
    loop3->type = Loop;
    loop3->next = NULL;
    loop3->inner = block8;
    
    Block *block9 = pushStruct(&context->blocks, Block);
    block9->type = Command;
    block9->next = loop3;
    
    Script *script4 = &context->scripts[context->scriptCount++];
    script4->id = 4;
    script4->x = 30;
    script4->y = -40;
    script4->topBlock = block9;
    
    block5->script = script4;
    block6->script = script4;
    block7->script = script4;
    loop2->script = script4;
    block8->script = script4;
    loop3->script = script4;
    block9->script = script4;
    
}

extern "C" BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input) {
    // Always reset the blocksCtx pointer in case we reloaded the dylib
    blocksCtx = (BlocksContext *)mem;
    BeginBlocks(*input);
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        Script *script = &blocksCtx->scripts[i];
        RenderScript(script);
    }
    return EndBlocks();
}
