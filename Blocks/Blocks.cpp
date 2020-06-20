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
b32 PointInRect(v2 point, BlocksRect rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

RenderEntry *PushRenderEntry(BlocksContext *ctx) {
    Assert(ctx->nextRenderingIdx - 1 < ArrayCount(ctx->renderEntries));
    RenderEntry *entry = &ctx->renderEntries[ctx->nextRenderingIdx - 1];
    entry->idx = ctx->nextRenderingIdx++;
    return entry;
}

void SetTopBlock(Script *script, Block *block) {
    script->topBlock = block;
    block->script = script;
}

Script *CreateScript(v2 position, Block *topBlock) {
    Script *script = &blocksCtx->scripts[blocksCtx->scriptCount++];
    *script = { 0 };
    script->P = position;
    
    // @TODO: We need to recursively go through all the blocks and set the new script pointer
    
    SetTopBlock(script, topBlock);
    return script;
}

Script *CreateScript(v2 position, BlockType type) {
    Script *script = &blocksCtx->scripts[blocksCtx->scriptCount++];
    *script = { 0 };
    script->P = position;
    
    Block *topBlock = PushStruct(&blocksCtx->blocks, Block);
    *topBlock = { 0 };
    topBlock->type = type;
    
    SetTopBlock(script, topBlock);
    return script;
}

Block *CreateBlock(Script *script, BlockType type) {
    Block *block = PushStruct(&blocksCtx->blocks, Block);
    *block = { 0 };
    block->type = type;
    block->script = script;
    return block;
}

void Connect(Block *from, Block *to) {
    // The `to` block always moves to `from`s script
    from->next = to;
    to->prev = from;
    to->script = from->script;
}

void ConnectInner(Block *from, Block *to) {
    Assert(from->type == BlockType_Loop);
    
    // The `to` block always moves to `from`s script
    from->inner = to;
    to->script = from->script;
    
    // @TODO: We need to recursively go through all the blocks and set the new script pointer
}

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear memory
    blocksCtx->verts.used = 0;
    
    blocksCtx->hot.renderingIdx = 0;
    blocksCtx->nextHot.renderingIdx = 0;
    
    blocksCtx->nextRenderingIdx = 1;
}

BlocksRenderInfo EndBlocks() {
    if (blocksCtx->interacting.renderingIdx) {
        if (!blocksCtx->input.mouseDown) {
            blocksCtx->interacting.renderingIdx = 0;
        }
        else {
            *blocksCtx->interacting.x = blocksCtx->input.mouseP.x - blocksCtx->interacting.mouseOffset.x;
            *blocksCtx->interacting.y = blocksCtx->input.mouseP.y - blocksCtx->interacting.mouseOffset.y;
        }
    }
    else {
        if (blocksCtx->nextHot.renderingIdx) {
            blocksCtx->hot = blocksCtx->nextHot;
        }
        if (blocksCtx->hot.renderingIdx && blocksCtx->input.mouseDown) {
            // Begin interacting
            blocksCtx->interacting = blocksCtx->hot;
            
            // // If we're in the middle of a stack, tear off into a new stack
            // Block *hotBlock = getBlockById(blocksCtx, blocksCtx->hot.blockId);
            
            // if (hotBlock != hotBlock->script->topBlock) {
            //     RenderEntry *hotEntry = &blocksCtx->renderEntries[blocksCtx->interacting.renderingIdx];
            //     hotBlock->prev->next = NULL;
            //     hotBlock->prev = NULL;
            //     v2 scriptP = { hotEntry->x, hotEntry->y };
            //     Script *script = CreateScript(scriptP, hotBlock);
            //     blocksCtx->interacting.x = &script->x;
            //     blocksCtx->interacting.y = &script->y;
            //     v2 mouseOffset = blocksCtx->interacting.mouseOffset;
            //     blocksCtx->interacting.mouseOffset = { mouseOffset.x - hotEntry->x, mouseOffset.y - hotEntry->y };
            // }
        }
    }
    
    // Change the color of the hot block
    if (blocksCtx->hot.renderingIdx) {
        blocksCtx->renderEntries[blocksCtx->hot.renderingIdx - 1].color = v3{1, 1, 0};
    }
    
    // Assemble vertex buffer
    for (u32 i = 0; i < blocksCtx->nextRenderingIdx - 1; ++i) {
        RenderEntry *entry = &blocksCtx->renderEntries[i];
        switch(entry->type) {
            case RenderEntryType_Command: {
                PushCommandBlockVerts(blocksCtx, entry->P, entry->color);
                break;
            }
            case RenderEntryType_Loop: {
                PushLoopBlockVerts(blocksCtx, entry->P, entry->color, entry->hStretch, entry->vStretch);
                break;
            }
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
    
    RenderBasis basis = { script->P, 0, 0 };
    DrawBlock(script->topBlock, &basis);
}

void DrawBlock(Block *block, RenderBasis *basis) {
    switch(block->type) {
        case BlockType_Command: {
            DrawCommandBlock(block, basis);
            break;
        }
        case BlockType_Loop: {
            DrawLoopBlock(block, basis);
        }
        default: break;
    }
}

void DrawCommandBlock(Block *block, RenderBasis *basis) {
    
    BlocksRect hitBox = { basis->at.x, basis->at.y, 16, 16};
    
    // PushCommandBlockVerts(blocksCtx, v2{basis->at.x, basis->at.y}, v3{0, 1, 1});
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Command;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{0, 1, 1};
    
    if (block->next) {
        basis->at.x += 16;
        DrawBlock(block->next, basis);
    }
    
    basis->bounds.x += 16;
    basis->bounds.y = Max(basis->bounds.y, 16);
    
    if (PointInRect(blocksCtx->input.mouseP, hitBox)) {
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.x = &block->script->P.x;
        blocksCtx->nextHot.y = &block->script->P.y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - block->script->P.x, blocksCtx->input.mouseP.y - block->script->P.y };
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
    BlocksRect innerHitBox = { basis->at.x + 6, basis->at.y, (f32)horizStretch + 16, (f32)vertStretch + 16 };
    
    // PushLoopBlockVerts(blocksCtx, v2{basis->at.x, basis->at.y}, v3{1, 0, 1}, horizStretch, vertStretch);
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Loop;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{1, 0, 1};
    entry->hStretch = horizStretch;
    entry->vStretch = vertStretch;
    
    if (block->next) {
        basis->at.x += 38 + horizStretch;
        DrawBlock(block->next, basis);
    }
    
    basis->bounds.x += 38 + horizStretch;
    basis->bounds.y = Max(basis->bounds.y, 20 + vertStretch);
    
    if (PointInRect(blocksCtx->input.mouseP, hitBox) && !PointInRect(blocksCtx->input.mouseP, innerHitBox)) {
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.x = &block->script->P.x;
        blocksCtx->nextHot.y = &block->script->P.y;
        
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - block->script->P.x, blocksCtx->input.mouseP.y - block->script->P.y };
    }
    
}


extern "C" void InitBlocks(void *mem, u32 memSize) {
    static const u32 VERTS_MEM_SIZE = 65535 * (7 * sizeof(f32));
    
    Assert(memSize >= sizeof(BlocksContext) + VERTS_MEM_SIZE);
    
    Arena dummyArena = {};
    dummyArena.data = (u8 *)mem;
    dummyArena.size = memSize;
    dummyArena.used = 0;
    
    BlocksContext *context = PushStruct(&dummyArena, BlocksContext);
    context->verts = SubArena(&dummyArena, VERTS_MEM_SIZE);
    
    // All the rest of the data block
    u32 blocksArenaSize = dummyArena.size - dummyArena.used;
    context->blocks = SubArena(&dummyArena, blocksArenaSize);
    
    context->scriptCount = 0;
    
    blocksCtx = context;
    
    // Create some blocks, y'know, for fun
    {
        // A script with a single command block
        CreateScript(v2{0, 0}, BlockType_Command);
    }
    
    {
        // A script with three blocks in a row
        Script *script = CreateScript(v2{40, -30}, BlockType_Command);
        Block *topBlock = script->topBlock;
        
        Block *loop = CreateBlock(script, BlockType_Loop);
        Connect(topBlock, loop);
        
        Block *otherBlock = CreateBlock(script, BlockType_Command);
        Connect(loop, otherBlock);
    }
    
    {
        // A script with nested loops
        Script *script = CreateScript(v2{-30, -40}, BlockType_Command);
        Block *block1 = script->topBlock;
        
        Block *outerLoop = CreateBlock(script, BlockType_Loop);
        Connect(block1, outerLoop);
        
            Block *block2 = CreateBlock(script, BlockType_Command);
            Block *innerLoop = CreateBlock(script, BlockType_Loop);
            Block *block3 = CreateBlock(script, BlockType_Command);
            
            Connect(block2, innerLoop);
            Connect(innerLoop, block3);
        
        ConnectInner(outerLoop, block2);
        
        Block *last = CreateBlock(script, BlockType_Command);
        Connect(outerLoop, last);
    }
    
    
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
