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

void DeleteScript(Script *script) {
    // Find this script in the array
    s32 scriptIdx = -1;
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        if (&blocksCtx->scripts[i] == script) {
            scriptIdx = i;
        }
    }
    Assert(scriptIdx != -1);
    
    // Swap the last script into this script's position to keep the array tightly packed
    if (scriptIdx < blocksCtx->scriptCount - 1) {
        blocksCtx->scripts[scriptIdx] = blocksCtx->scripts[blocksCtx->scriptCount - 1];
    }
    blocksCtx->scriptCount--;
}

Block *CreateBlock(BlockType type) {
    Block *block = PushStruct(&blocksCtx->blocks, Block);
    *block = { 0 };
    block->type = type;
    return block;
}

inline
void Connect(Block *from, Block *to) {
    from->next = to;
    to->prev = from;
}

inline
void ConnectInner(Block *from, Block *to) {
    Assert(from->type == BlockType_Loop);
    from->inner = to;
    to->parent = from;
}

// Call this on the block to disconnect from its previous
inline
void Disconnect(Block *block) {
    Assert(block->prev);
    block->prev->next = NULL;
    block->prev = NULL;
}

// Call this on the block to disconnect from its parent
inline
void DisconnectInner(Block *block) {
    Assert(block->parent);
    block->parent->inner = NULL;
    block->parent = NULL;
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

inline
b32 IsTopBlock(Block *block) {
    // Simple linear search
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        if (blocksCtx->scripts[i].topBlock == block) {
            return true;
        }
    }
    return false;
}

inline
b32 IsTopBlockOfScript(Block *block, Script *script) {
    Assert(block && script);
    return (script->topBlock == block);
}

inline
b32 Interacting() {
    return blocksCtx->interacting.type != InteractionType_None;
}

inline
b32 Dragging() {
    return blocksCtx->interacting.type == InteractionType_Drag;
}

inline
Layout CreateEmptyLayoutAt(v2 at) {
    return {at, Rectangle{at.x, at.y, 0, 0}};
}

inline
Layout CreateEmptyLayoutAt(f32 x, f32 y) {
    return {v2{x, y}, Rectangle{x, y, 0, 0}};
}

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear memory
    blocksCtx->verts.used = 0;
    
    blocksCtx->hot.block = 0;
    blocksCtx->nextHot.block = 0;
    
    blocksCtx->nextRenderingIdx = 0;
}

BlocksRenderInfo EndBlocks() {
    if (Interacting()) {
        Interaction *interact = &blocksCtx->interacting;
        
        if (!blocksCtx->input.mouseDown) {
            // End interaction
            switch(interact->type) {
                case InteractionType_Select: {
                    // @TODO: Eventually, toggle script thread execution
                    break;
                }
                case InteractionType_Drag: {
                    DragInfo dragInfo = blocksCtx->dragInfo;
                    // Drop and combine stacks as necessary
                    if (dragInfo.readyToInsert) {
                        // @NOTE: We *always* keep the script we're adding the dragging blocks to and throw out the dragging script
                        switch(dragInfo.insertionType) {
                            case InsertionType_Before: {
                                Connect(dragInfo.lastBlock, dragInfo.insertionBaseBlock);
                                dragInfo.insertionBaseScript->topBlock = dragInfo.firstBlock;
                                dragInfo.insertionBaseScript->P.x -= dragInfo.scriptLayout.bounds.w;
                                DeleteScript(dragInfo.script);
                                break;
                            }
                            case InsertionType_After: {
                                Block *next = dragInfo.insertionBaseBlock->next;
                                if (next) {
                                    Disconnect(next);
                                }
                                Connect(dragInfo.insertionBaseBlock, dragInfo.firstBlock);
                                if (next) {
                                    if (dragInfo.firstBlock->type == BlockType_Loop && !dragInfo.firstBlock->inner) {
                                        ConnectInner(dragInfo.firstBlock, next);
                                    }
                                    else {
                                        Connect(dragInfo.lastBlock, next);
                                    }
                                }
                                DeleteScript(dragInfo.script);
                                break;
                            }
                            case InsertionType_Inside: {
                                Block *inner = dragInfo.insertionBaseBlock->inner;
                                if (inner) {
                                    DisconnectInner(inner);
                                }
                                ConnectInner(dragInfo.insertionBaseBlock, dragInfo.firstBlock);
                                if (inner) {
                                    if (dragInfo.firstBlock->type == BlockType_Loop && !dragInfo.firstBlock->inner) {
                                        ConnectInner(dragInfo.firstBlock, inner);
                                    }
                                    else {
                                        Connect(dragInfo.lastBlock, inner);
                                    }
                                }
                                DeleteScript(dragInfo.script);
                                break;
                            }
                            case InsertionType_Around: {
                                Assert(dragInfo.firstBlock->type == BlockType_Loop && !dragInfo.firstBlock->inner);
                                ConnectInner(dragInfo.firstBlock, dragInfo.insertionBaseBlock);
                                dragInfo.insertionBaseScript->topBlock = dragInfo.firstBlock;
                                dragInfo.insertionBaseScript->P.x -= 6;
                                DeleteScript(dragInfo.script);
                                break;
                            }
                        }
                    }
                    
                    break;
                }
                default: { break; }
            }
            
            blocksCtx->interacting = {};
        }
        else {
            // Update interaction
            switch(interact->type) {
                case InteractionType_Select: {
                    // If the mouse has moved far enough, start dragging
                    if (DistSq(blocksCtx->input.mouseP, interact->mouseStartP) > MIN_DRAG_DIST * MIN_DRAG_DIST) {
                        interact->type = InteractionType_Drag;
                        
                        // If we're in the middle of a stack, tear off into a new stack
                        Block *hotBlock = interact->block;
                        if (!IsTopBlock(hotBlock)) {
                            Script *newScript = TearOff(hotBlock, interact->blockP);
                            interact->script = newScript;
                            interact->mouseOffset = { interact->mouseStartP.x - interact->blockP.x, interact->mouseStartP.y - interact->blockP.y };
                        }
                        
                        // Set constant dragInfo
                        Script *script = blocksCtx->interacting.script;
                        blocksCtx->dragInfo.script = script;
                        blocksCtx->dragInfo.firstBlock = script->topBlock;
                        Block *lastBlock = script->topBlock;
                        while (lastBlock->next) {
                            lastBlock = lastBlock->next;
                        }
                        blocksCtx->dragInfo.lastBlock = lastBlock;
                    }
                    break;
                }
                case InteractionType_Drag: {
                    Script *script = interact->script;
                    script->P.x = blocksCtx->input.mouseP.x - interact->mouseOffset.x;
                    script->P.y = blocksCtx->input.mouseP.y - interact->mouseOffset.y;
                    
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
    
    if (Dragging()) {
        PushSimpleRect(blocksCtx, blocksCtx->dragInfo.inlet, v3{0, 1, 0});
        PushSimpleRect(blocksCtx, blocksCtx->dragInfo.outlet, v3{0, 1, 0});
        if (blocksCtx->interacting.block->type == BlockType_Loop) {
            PushSimpleRect(blocksCtx, blocksCtx->dragInfo.innerOutlet, v3{0, 1, 0});
        }
    }
    
    BlocksRenderInfo Result;
    Result.verts = blocksCtx->verts.data;
    Result.vertsCount = blocksCtx->verts.used / VERTEX_SIZE;
    Result.vertsSize = blocksCtx->verts.used;
    return Result;
}

Layout RenderScript(Script *script) {
    Assert(script->topBlock);
    
    Layout layout = CreateEmptyLayoutAt(script->P);
    DrawSubScript(script->topBlock, script, &layout);
    return layout;
}

void DrawSubScript(Block *block, Script *script, Layout *layout) {
    Assert(block);
    // Draw a single linear group of blocks, only recursing on branching blocks
    Rectangle inletBounds = {layout->at.x - 4, layout->at.y, 8, 16};
    Block *nextBlock = block;
    while (nextBlock) {
        if(DrawBlock(nextBlock, script, layout)) {
            nextBlock = nextBlock->next;
        }
        else {
            nextBlock = 0;
        }
    }
    
    if (Dragging() && !blocksCtx->dragInfo.readyToInsert) {
        DragInfo dragInfo = blocksCtx->dragInfo;
        if (dragInfo.script != script && dragInfo.firstBlock->type == BlockType_Loop && !dragInfo.script->topBlock->inner) {
            if (RectsIntersect(inletBounds, dragInfo.innerOutlet)) {
                Layout loopLayout = CreateEmptyLayoutAt(layout->bounds.origin.x - 6, layout->bounds.origin.y);
                DrawGhostLoopBlock(&loopLayout, layout);
                blocksCtx->dragInfo.readyToInsert = true;
                blocksCtx->dragInfo.insertionType = InsertionType_Around;
                blocksCtx->dragInfo.insertionBaseBlock = block;
                blocksCtx->dragInfo.insertionBaseScript = script;
                PushSimpleRect(blocksCtx, loopLayout.bounds, {0, 1, 0});
            }
        }
    }
    
    PushSimpleRect(blocksCtx, layout->bounds, {0, 1, 0});
}

b32 DrawBlock(Block *block, Script *script, Layout *layout) {
    DragInfo dragInfo = blocksCtx->dragInfo;
    
    // Draw ghost block before this block, if necessary
    if (Dragging() && dragInfo.script != script && IsTopBlockOfScript(block, script) && !blocksCtx->dragInfo.readyToInsert) {
        Rectangle inletBounds = {script->P.x - 4, script->P.y, 8, 16};
        PushSimpleRect(blocksCtx, inletBounds, v3{1, 0, 0});
        if (RectsIntersect(inletBounds, dragInfo.outlet)) {
            switch (dragInfo.lastBlock->type) {
                case BlockType_Command: {
                    layout->at.x -= 16;
                    layout->bounds.x -= 16;
                    DrawGhostCommandBlock(layout);
                    break;
                }
                case BlockType_Loop: {
                    layout->at.x -= 38;
                    layout->bounds.x -= 38;
                    DrawGhostLoopBlock(layout);
                    break;
                }
            }
            blocksCtx->dragInfo.readyToInsert = true;
            blocksCtx->dragInfo.insertionType = InsertionType_Before;
            blocksCtx->dragInfo.insertionBaseBlock = block;
            blocksCtx->dragInfo.insertionBaseScript = script;
        }
    }
    
    switch(block->type) {
        case BlockType_Command: {
            DrawCommandBlock(block, script, layout);
            break;
        }
        case BlockType_Loop: {
            Layout innerLayout = CreateEmptyLayoutAt(layout->at.x + 6, layout->at.y);
            b32 renderedInner = false;
            
            // Draw ghost block inside the loop, if necessary
            if (Dragging() && dragInfo.script != script && !blocksCtx->dragInfo.readyToInsert) {
                Rectangle innerOutletBounds = {layout->at.x + 3, layout->at.y, 6, 16};
                PushSimpleRect(blocksCtx, innerOutletBounds, v3{1, 0, 0});
                if (RectsIntersect(innerOutletBounds, dragInfo.inlet)) {
                    switch (dragInfo.firstBlock->type) {
                        case BlockType_Command: {
                            DrawGhostCommandBlock(&innerLayout);
                            break;
                        }
                        case BlockType_Loop: {
                            // Override block drawing so that loop contains the rest of the substack
                            Layout innerInnerLayout = CreateEmptyLayoutAt(innerLayout.at.x + 6, innerLayout.at.y);
                            if (block->inner) {
                                DrawSubScript(block->inner, script, &innerInnerLayout);
                            }
                            DrawGhostLoopBlock(&innerLayout, &innerInnerLayout);
                            renderedInner = true;
                        }
                    }
                    blocksCtx->dragInfo.readyToInsert = true;
                    blocksCtx->dragInfo.insertionType = InsertionType_Inside;
                    blocksCtx->dragInfo.insertionBaseBlock = block;
                    blocksCtx->dragInfo.insertionBaseScript = script;
                }
            }
            
            // If we didn't already draw the entire inner script (i.e., with a ghost block)
            // then do it now, the normal way
            if (block->inner && !renderedInner) {
                DrawSubScript(block->inner, script, &innerLayout);
            }
            DrawLoopBlock(block, script, layout, &innerLayout);
            
            break;
        }
        default: break;
    }
    
    // Draw ghost block after this block, if necessary
    if (Dragging() && dragInfo.script != script && !blocksCtx->dragInfo.readyToInsert) {
        Rectangle outletBounds = {layout->at.x - 4, layout->at.y, 8, 16};
        PushSimpleRect(blocksCtx, outletBounds, v3{1, 0, 0});
        if (RectsIntersect(outletBounds, dragInfo.inlet)) {
            switch (dragInfo.firstBlock->type) {
                case BlockType_Command: {
                    DrawGhostCommandBlock(layout);
                    blocksCtx->dragInfo.readyToInsert = true;
                    blocksCtx->dragInfo.insertionType = InsertionType_After;
                    blocksCtx->dragInfo.insertionBaseBlock = block;
                    blocksCtx->dragInfo.insertionBaseScript = script;
                    break;
                }
                case BlockType_Loop: {
                    if (dragInfo.script->topBlock->inner) {
                        // If the loop already contains an inner stack, just put it in line
                        DrawGhostLoopBlock(layout);
                        blocksCtx->dragInfo.readyToInsert = true;
                        blocksCtx->dragInfo.insertionType = InsertionType_After;
                        blocksCtx->dragInfo.insertionBaseBlock = block;
                        blocksCtx->dragInfo.insertionBaseScript = script;
                    }
                    else {
                        // Otherwise, override block drawing so that loop contains the rest of the substack
                        Layout innerLayout = CreateEmptyLayoutAt(layout->at.x + 6, layout->at.y);
                        if (block->next) {
                            DrawSubScript(block->next, script, &innerLayout);
                        }
                        DrawGhostLoopBlock(layout, &innerLayout);
                        blocksCtx->dragInfo.readyToInsert = true;
                        blocksCtx->dragInfo.insertionType = InsertionType_After;
                        blocksCtx->dragInfo.insertionBaseBlock = block;
                        blocksCtx->dragInfo.insertionBaseScript = script;
                        
                        // @TODO: I don't love this weird return boolean thing. Is there a way to avoid this?
                        return false; // Don't continue drawing this substack
                    }
                    break;
                }
            }
        }
    }
    
    return true;
}

void DrawCommandBlock(Block *block, Script *script, Layout *layout, u32 flags) {
    b32 isGhost = flags & DrawBlockFlags_Ghost;
    
    Rectangle hitBox = { layout->at.x, layout->at.y, 16, 16};
    
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Command;
    entry->block = block;
    entry->P = v2{layout->at.x, layout->at.y};
    if (isGhost) {
        entry->color = v3{0.5, 0.5, 0.5};
    }
    else {
        entry->color = v3{0, 0.5, 0.5};
    }
    
    
    layout->at.x += 16;

    layout->bounds.w += 16;
    layout->bounds.h = Max(layout->bounds.h, 16);
    
    if (!isGhost && PointInRect(blocksCtx->input.mouseP, hitBox)) {
        blocksCtx->nextHot.type = InteractionType_Select;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.mouseStartP = blocksCtx->input.mouseP;
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - script->P.x, blocksCtx->input.mouseP.y - script->P.y };
    }
    
}

void DrawLoopBlock(Block *block, Script *script, Layout *layout, Layout *innerLayout, u32 flags) {
    b32 isGhost = flags & DrawBlockFlags_Ghost;
    
    u32 horizStretch = 0;
    u32 vertStretch = 0;
    if (innerLayout) {
        horizStretch = Max(innerLayout->bounds.w - 16, 0);
        vertStretch = Max(innerLayout->bounds.h - 16, 0);
    }
    
        
    Rectangle hitBox = { layout->at.x, layout->at.y, 38 + (f32)horizStretch, 20 + (f32)vertStretch };
    Rectangle innerHitBox = { layout->at.x + 6, layout->at.y, (f32)horizStretch + 16, (f32)vertStretch + 16 };
    
    RenderEntry *entry = PushRenderEntry(blocksCtx);
    entry->type = RenderEntryType_Loop;
    entry->block = block;
    entry->P = v2{layout->at.x, layout->at.y};
    if (isGhost) {
        entry->color = v3{0.5, 0.5, 0.5};
    }
    else {
        entry->color = v3{0.5, 0, 0.5};
    }
    entry->hStretch = horizStretch;
    entry->vStretch = vertStretch;
    
    layout->at.x += 38 + horizStretch;
    
    layout->bounds.w += 38 + horizStretch;
    layout->bounds.h = Max(layout->bounds.h, 20 + vertStretch);
    
    if (!isGhost && PointInRect(blocksCtx->input.mouseP, hitBox) && !PointInRect(blocksCtx->input.mouseP, innerHitBox)) {
        blocksCtx->nextHot.type = InteractionType_Select;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.renderingIdx = entry->idx;
        blocksCtx->nextHot.mouseStartP = blocksCtx->input.mouseP;
        blocksCtx->nextHot.mouseOffset = { blocksCtx->input.mouseP.x - script->P.x, blocksCtx->input.mouseP.y - script->P.y };
    }
}

inline
void DrawGhostCommandBlock(Layout *layout) {
    DrawCommandBlock(NULL, NULL, layout, DrawBlockFlags_Ghost);
}

inline
void DrawGhostLoopBlock(Layout *layout, Layout *innerLayout) {
    DrawLoopBlock(NULL, NULL, layout, innerLayout, DrawBlockFlags_Ghost);
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
    if (Dragging()) {
        // Update dragging info
        Script *script = blocksCtx->dragInfo.script;
        Layout dragLayout = RenderScript(script);
        blocksCtx->dragInfo.scriptLayout = dragLayout;
        blocksCtx->dragInfo.inlet = {script->P.x - 4, script->P.y, 8, 16};
        blocksCtx->dragInfo.outlet = {dragLayout.at.x - 4, dragLayout.at.y, 8, 16};
        if (blocksCtx->interacting.block->type == BlockType_Loop) {
            blocksCtx->dragInfo.innerOutlet = {script->P.x + 3, script->P.y, 6, 16};
        }
        
        // Reset this to false each frame so we can update the ghost block insertion point, if necessary
        blocksCtx->dragInfo.readyToInsert = false;
    }
     for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        Script *script = &blocksCtx->scripts[i];
        if (Dragging() && blocksCtx->dragInfo.script == script) {
            continue;
        }
        RenderScript(script);
    }
    return EndBlocks();
}
