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
#include "BlocksMath.h"
#include "BlocksVerts.h"

#define MIN_DRAG_DIST 4.0

global_var BlocksContext *blocksCtx = 0;

inline
b32 PointInRect(v2 point, BlocksRect rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

RenderEntry *PushRenderEntry(BlocksContext *ctx) {
    Assert(ctx->nextRenderingIdx < ArrayCount(ctx->renderEntries));
    RenderEntry *entry = &ctx->renderEntries[ctx->nextRenderingIdx];
    entry->idx = ctx->nextRenderingIdx++;
    return entry;
}

Script *CreateScript(v2 position) {
    Script *script = &blocksCtx->scripts[blocksCtx->scriptCount++];
    *script = { 0 };
    script->P = position;
    return script;
}

Block *CreateBlock(BlockType type) {
    Block *block = PushStruct(&blocksCtx->blocks, Block);
    *block = { 0 };
    block->type = type;
    return block;
}

void Connect(Block *from, Block *to) {
    from->next = to;
    to->prev = from;
}

void ConnectInner(Block *from, Block *to) {
    Assert(from->type == BlockType_Loop);
    from->inner = to;
    to->parent = from;
}

Script *TearOff(Block *block, v2 position) {
    Assert(block->prev || block->parent);
    Script *script = CreateScript(position);
    if (block->prev) {
        block->prev->next = NULL;
        block->prev = NULL;
    }
    else if (block->parent) {
        block->parent->inner = NULL;
        block->parent = NULL;
    }
    script->topBlock = block;
    return script;
}

b32 IsTopBlockOfScript(Block *block) {
    // Simple linear search
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        if (blocksCtx->scripts[i].topBlock == block) {
            return true;
        }
    }
    return false;
}

inline
b32 Interacting() {
    return blocksCtx->interacting.type != InteractionType_None;
}

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear memory
    blocksCtx->verts.used = 0;
    
    blocksCtx->hot.block = 0;
    blocksCtx->nextHot.block = 0;
    
    blocksCtx->nextRenderingIdx = 0;
    
    // blocksCtx->ghostBlockParent = 0;
}

BlocksRenderInfo EndBlocks() {
    if (Interacting()) {
        if (!blocksCtx->input.mouseDown) {
            // Stop interaction
            blocksCtx->interacting = {};
            blocksCtx->ghostBlockParent = 0;
        }
        else {
            // Update interaction
            Interaction *interact = &blocksCtx->interacting;
            Assert(interact->type != InteractionType_None)
            switch(interact->type) {
                case InteractionType_Select: {
                    // If the mouse has moved far enough, start dragging
                    if (DistSq(blocksCtx->input.mouseP, interact->mouseStartP) > MIN_DRAG_DIST * MIN_DRAG_DIST) {
                        interact->type = InteractionType_Drag;
                        
                        // If we're in the middle of a stack, tear off into a new stack
                        Block *hotBlock = interact->block;
                        if (!IsTopBlockOfScript(hotBlock)) {
                            Script *newScript = TearOff(hotBlock, interact->blockP);
                            interact->script = newScript;
                            interact->mouseOffset = { interact->mouseStartP.x - interact->blockP.x, interact->mouseStartP.y - interact->blockP.y };
                        }
                    }
                    break;
                }
                case InteractionType_Drag: {
                    Script *script = interact->script;
                    script->P.x = blocksCtx->input.mouseP.x - interact->mouseOffset.x;
                    script->P.y = blocksCtx->input.mouseP.y - interact->mouseOffset.y;
                    
                    for (u32 i = 0; i < blocksCtx->nextRenderingIdx; ++i) {
                        RenderEntry *entry = &blocksCtx->renderEntries[i];
                        switch(entry->type) {
                            case RenderEntryType_Command: {
                                BlocksRect dropZone = { entry->P.x + 16, entry->P.y, 6, 16 };
                                if (PointInRect(script->P, dropZone)) {
                                    PushSimpleRect(blocksCtx, dropZone, v3{1, 0, 0} );
                                    blocksCtx->ghostBlockParent = entry->block;
                                    blocksCtx->ghostBlockType = interact->block->type;
                                    blocksCtx->ghostBlockInsert = GhostBlockInsert_After;
                                }
                                break;
                            }
                            case RenderEntryType_Loop: {
                                BlocksRect dropZoneInner = { entry->P.x + 6, entry->P.y, 6, 16 };
                                if (PointInRect(script->P, dropZoneInner)) {
                                    PushSimpleRect(blocksCtx, dropZoneInner, v3{1, 0, 0} );
                                }
                                
                                BlocksRect dropZoneNext = { entry->P.x + 38 + entry->hStretch, entry->P.y, 6, 16 };
                                if (PointInRect(script->P, dropZoneNext)) {
                                    PushSimpleRect(blocksCtx, dropZoneNext, v3{1, 0, 0} );
                                    blocksCtx->ghostBlockParent = entry->block;
                                    blocksCtx->ghostBlockType = interact->block->type;
                                    blocksCtx->ghostBlockInsert = GhostBlockInsert_After;
                                }
                                break;
                            }
                        }
                    }
                    
                    break;
                }
                default: { break; }
            }
        }
    }
    else {
        if (blocksCtx->nextHot.block) {
            blocksCtx->hot = blocksCtx->nextHot;
        }
        if (blocksCtx->hot.block && blocksCtx->input.mouseDown) {
            // Begin interaction
            blocksCtx->interacting = blocksCtx->hot;
        }
    }
    
    // Change the color of the hot block
    if (blocksCtx->hot.block) {
        blocksCtx->renderEntries[blocksCtx->hot.renderingIdx].color = v3{1, 1, 0};
    }
    
    // Assemble vertex buffer
    for (u32 i = 0; i < blocksCtx->nextRenderingIdx; ++i) {
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
    Result.vertsCount = blocksCtx->verts.used / VERTEX_SIZE;
    Result.vertsSize = blocksCtx->verts.used;
    return Result;
}

void RenderScript(Script *script) {
    if (!script->topBlock) { return; }
    
    RenderBasis basis = { script->P, 0, 0 };
    DrawBlock(script->topBlock, script, &basis);
}

void DrawGhostCommandBlock(RenderBasis *basis) {
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Command;
    entry->block = NULL;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{0.5, 0.5, 0.5};
    
    basis->at.x += 16;
    
    basis->bounds.x += 16;
    basis->bounds.y = Max(basis->bounds.y, 16);
}

void DrawGhostLoopBlock(RenderBasis *basis) {
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Loop;
    entry->block = NULL;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{0.5, 0.5, 0.5};
    entry->hStretch = 0;
    entry->vStretch = 0;
    
    basis->at.x += 38;
    
    basis->bounds.x += 38;
    basis->bounds.y = Max(basis->bounds.y, 20);
}

void DrawBlock(Block *block, Script *script, RenderBasis *basis) {
    switch(block->type) {
        case BlockType_Command: {
            DrawCommandBlock(block, script, basis);
            break;
        }
        case BlockType_Loop: {
            DrawLoopBlock(block, script, basis);
        }
        default: break;
    }
}

void DrawCommandBlock(Block *block, Script *script, RenderBasis *basis) {
    
    if (block == blocksCtx->ghostBlockParent && blocksCtx->ghostBlockInsert == GhostBlockInsert_Before) {
        // Draw ghost block before this block (which should only happen with the top block in a stack)
        switch(blocksCtx->ghostBlockType) {
            case BlockType_Command: {
                basis->at.x -= 16;
                DrawGhostCommandBlock(basis);
                break;
            }
            case BlockType_Loop: {
                basis->at.x -= 38;
                DrawGhostLoopBlock(basis);
                break;
            }
        }
    }
    
    BlocksRect hitBox = { basis->at.x, basis->at.y, 16, 16};
    
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Command;
    entry->block = block;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{0, 1, 1};
    
    basis->at.x += 16;
    
    if (block == blocksCtx->ghostBlockParent && blocksCtx->ghostBlockInsert == GhostBlockInsert_After) {
        // Draw ghost block after this block (and before the next blocks)
        switch(blocksCtx->ghostBlockType) {
            case BlockType_Command: {
                DrawGhostCommandBlock(basis);
                break;
            }
            case BlockType_Loop: {
                DrawGhostLoopBlock(basis);
                break;
            }
        }
    }
    
    if (block->next) {
        DrawBlock(block->next, script, basis);
    }
    
    basis->bounds.x += 16;
    basis->bounds.y = Max(basis->bounds.y, 16);
    
    if (PointInRect(blocksCtx->input.mouseP, hitBox)) {
        blocksCtx->nextHot.type = InteractionType_Select;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.mouseStartP = blocksCtx->input.mouseP;
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - script->P.x, blocksCtx->input.mouseP.y - script->P.y };
    }
    
}

void DrawLoopBlock(Block *block, Script *script, RenderBasis *basis) {
    
    u32 horizStretch = 0;
    u32 vertStretch = 0;
    if (block->inner) {
        RenderBasis innerBasis = { basis->at.x + 6, basis->at.y, 0, 0 };
        DrawBlock(block->inner, script, &innerBasis);
        horizStretch = (u32)(innerBasis.bounds.w - 16);
        vertStretch = (u32)(innerBasis.bounds.h - 16);
    }
    
    BlocksRect hitBox = { basis->at.x, basis->at.y, 38 + (f32)horizStretch, 20 + (f32)vertStretch };
    BlocksRect innerHitBox = { basis->at.x + 6, basis->at.y, (f32)horizStretch + 16, (f32)vertStretch + 16 };
    
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Loop;
    entry->block = block;
    entry->P = v2{basis->at.x, basis->at.y};
    entry->color = v3{1, 0, 1};
    entry->hStretch = horizStretch;
    entry->vStretch = vertStretch;
    
    basis->at.x += 38 + horizStretch;
    
    if (blocksCtx->ghostBlockParent && block == blocksCtx->ghostBlockParent) {
        switch(blocksCtx->ghostBlockInsert) {
            case GhostBlockInsert_Before: {
                break;
            }
            case GhostBlockInsert_After: {
                // Render the block immediately, then continue rendering as normal
                switch(blocksCtx->ghostBlockType) {
                    case BlockType_Command: {
                        DrawGhostCommandBlock(basis);
                        break;
                    }
                    case BlockType_Loop: {
                        DrawGhostLoopBlock(basis);
                        break;
                    }
                }
                break;
            }
            case GhostBlockInsert_Inner: {
                break;
            }
        }
    }
    
    
    if (block->next) {
        DrawBlock(block->next, script, basis);
    }
    
    basis->bounds.x += 38 + horizStretch;
    basis->bounds.y = Max(basis->bounds.y, 20 + vertStretch);
    
    if (PointInRect(blocksCtx->input.mouseP, hitBox) && !PointInRect(blocksCtx->input.mouseP, innerHitBox)) {
        blocksCtx->nextHot.type = InteractionType_Select;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.mouseStartP = blocksCtx->input.mouseP;
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - script->P.x, blocksCtx->input.mouseP.y - script->P.y };
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
        Script *script = CreateScript(v2{0, 0});
        Block *block = CreateBlock(BlockType_Command);
        script->topBlock = block;
    }
    
    {
        // A script with three blocks in a row
        Script *script = CreateScript(v2{40, -30});
        Block *topBlock = CreateBlock(BlockType_Command);
        script->topBlock = topBlock;
        
        Block *loop = CreateBlock(BlockType_Loop);
        Connect(topBlock, loop);
        
        Block *block2 = CreateBlock(BlockType_Command);
        Connect(loop, block2);
    }
    
    
    {
        // A script with nested loops
        Script *script = CreateScript(v2{-30, 50});
        Block *block1 = CreateBlock(BlockType_Command);
        script->topBlock = block1;
        
        Block *outerLoop = CreateBlock(BlockType_Loop);
        Connect(block1, outerLoop);
        
            Block *block2 = CreateBlock(BlockType_Command);
            Block *innerLoop = CreateBlock(BlockType_Loop);
            Block *block3 = CreateBlock(BlockType_Command);
            
            Connect(block2, innerLoop);
            Connect(innerLoop, block3);
        
        ConnectInner(outerLoop, block2);
        
        Block *last = CreateBlock(BlockType_Command);
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
