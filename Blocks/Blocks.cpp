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
#include "BlocksShaders.h"

#define MIN_DRAG_DIST 4.0

global_var BlocksContext *blocksCtx = 0;

RenderEntry *PushRenderEntry(RenderGroup *group) {
    Assert(group->entryCount < ArrayCount(group->entries));
    RenderEntry *entry = &group->entries[group->entryCount++];
    return entry;
}

void DEBUGPushRectOutline(Rectangle rect, v4 color) {
    RenderEntry *entry = PushRenderEntry(&blocksCtx->debugRenderGroup);
    entry->type = RenderEntryType_RectOutline;
    entry->rect = rect;
    entry->color = color;
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
    Block *block = PushStruct(&blocksCtx->permanent, Block);
    *block = { 0 };
    block->type = type;
    return block;
}

inline
b32 HasOutlet(BlockType type) {
    switch (type) {
        case BlockType_Command: return true;
        case BlockType_Event:   return true;
        case BlockType_EndCap:  return false;
        case BlockType_Loop:    return true;
        case BlockType_Forever: return false;
        default: break;
    }
    return true; // I guess? Since most block do have an outlet?
}

inline
b32 HasInlet(BlockType type) {
    switch (type) {
        case BlockType_Command: return true;
        case BlockType_Event:   return false;
        case BlockType_EndCap:  return true;
        case BlockType_Loop:    return true;
        case BlockType_Forever: return true;
        default: break;
    }
    return true; // I guess? Since most block do have an outlet?
}

inline
b32 HasInnerOutlet(BlockType type) {
    switch (type) {
        case BlockType_Command: return false;
        case BlockType_Event:   return false;
        case BlockType_EndCap:  return false;
        case BlockType_Loop:    return true;
        case BlockType_Forever: return true;
        default: break;
    }
    return true; // I guess? Since most block do have an outlet?
}

inline
void Connect(Block *from, Block *to) {
    Assert(HasOutlet(from->type));
    Assert(HasInlet(to->type));
    from->next = to;
    to->prev = from;
}

inline
void ConnectInner(Block *from, Block *to) {
    Assert(HasInnerOutlet(from->type));
    Assert(HasInlet(to->type));
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
    return blocksCtx->interacting.type == InteractionType_BlockDrag;
}

inline
b32 IsSimpleBlockType(BlockType type) {
    return type == BlockType_Command || type == BlockType_Event || type == BlockType_EndCap;
}

inline
b32 IsBranchBlockType(BlockType type) {
    return type == BlockType_Loop || type == BlockType_Forever;
}

inline
Layout CreateEmptyLayoutAt(v2 at) {
    return {at, Rectangle{at.x, at.y, 0, 0}};
}

inline
Layout CreateEmptyLayoutAt(f32 x, f32 y) {
    return {v2{x, y}, Rectangle{x, y, 0, 0}};
}

inline
RenderEntryType RenderEntryTypeForBlockType(BlockType blockType) {
    switch (blockType) {
        case BlockType_Command: return RenderEntryType_Command;
        case BlockType_Event:   return RenderEntryType_Event;
        case BlockType_EndCap:  return RenderEntryType_EndCap;
        case BlockType_Loop:    return RenderEntryType_Loop;
        case BlockType_Forever: return RenderEntryType_Forever;
        default: break;
    }
    return RenderEntryType_Null;
}

inline
v4 ColorForBlockType(BlockType blockType) {
    switch (blockType) {
        case BlockType_Command: return HexToColor(0x4C97FF);
        case BlockType_Event:   return HexToColor(0xFFD500);
        case BlockType_EndCap:  return HexToColor(0x9966FF);
        case BlockType_Loop:    return HexToColor(0xFFAB19);
        case BlockType_Forever: return HexToColor(0xD65CD6);
        default: break;
    }
    return v4{1, 1, 1, 1};
}

v2 UnprojectMouse(v2 mouseP, RenderGroup *group) {
    f32 projectedX = (((2.0 * mouseP.x) / blocksCtx->screenSize.w) - 1.0);
    f32 projectedY = (((2.0 * mouseP.y) / blocksCtx->screenSize.h) - 1.0);
    
    mat4x4 unprojection = group->invTransform;
    v4 unprojectedP = unprojection * v4{projectedX, projectedY, 0, 1};
    return unprojectedP.xy;
}

void InitRenderGroup(RenderGroup *group, mat4x4 transform, mat4x4 invTransform) {
    group->entryCount = 0;
    group->transform = transform;
    group->invTransform = invTransform;
    group->mouseP = UnprojectMouse(blocksCtx->input.mouseP, group);
}

void AssembleVertexBuferForRenderGroup(Arena *vertexArena, BlocksRenderInfo *renderInfo, RenderGroup* renderGroup) {
    Assert(renderInfo->drawCallCount < ArrayCount(renderInfo->drawCalls));
    
    BlocksDrawCall *drawCall = &renderInfo->drawCalls[renderInfo->drawCallCount++];
    drawCall->transform = renderGroup->transform;
    
    u8 *start = ArenaAt(vertexArena);
    drawCall->vertexOffset = (u32)(start - renderInfo->vertexData) / VERTEX_SIZE;
    
    if (renderGroup == &blocksCtx->fontRenderGroup) {
        v2 at = {-50, 0};
        const char *str = "AVA To Hello world!";
        for (u32 i = 0; i < strlen(str); ++i) {
            SdfFontChar c = FONT_DATA[str[i]];
            PushChar(vertexArena, c, at, COLOR_GREEN);
            at.x += c.advance;
            if (i < strlen(str) - 1) {
                f32 kern = KERN_TABLE[str[i + 1]][str[i]];
                at.x += kern;
            }
        }
        
    }
    
    for (u32 entryIdx = 0; entryIdx < renderGroup->entryCount; ++entryIdx) {
        RenderEntry *entry = &renderGroup->entries[entryIdx];
        switch(entry->type) {
            case RenderEntryType_Command: {
                PushCommandBlockVerts(vertexArena, entry->P, entry->color, entry->scale);
                break;
            }
            case RenderEntryType_Event: {
                PushEventBlockVerts(vertexArena, entry->P, entry->color);
                break;
            }
            case RenderEntryType_EndCap: {
                PushEndCapBlockVerts(vertexArena, entry->P, entry->color);
                break;
            }
            case RenderEntryType_Loop: {
                PushLoopBlockVerts(vertexArena, entry->P, entry->color, entry->hStretch, entry->vStretch);
                break;
            }
            case RenderEntryType_Forever: {
                PushForeverBlockVerts(vertexArena, entry->P, entry->color, entry->hStretch, entry->vStretch);
                break;
            }
            case RenderEntryType_Rect: {
                PushSolidRect(vertexArena, entry->rect, entry->color);
                break;
            }
            case RenderEntryType_RectOutline: {
                PushRectOutline(vertexArena, entry->rect, entry->color);
                break;
            }
            case RenderEntryType_Null: {
                // No-op
                // @TODO: Assert? Warning? Nothing?
                break;
            }
        }
        
        #if 0
        // Draw inlet, outlet, and innerOutlet hit-boxes
        if (entry->block) {
            BlockType type = entry->block->type;
            BlockMetrics metrics = METRICS[type];
            if (HasInlet(type)) {
                Rectangle inletRect = {entry->P.x + metrics.inlet.origin.x, 
                                       entry->P.y + metrics.inlet.origin.y,
                                       metrics.inlet.size.w,
                                       metrics.inlet.size.h};
                DEBUGPushRectOutline(inletRect, v4{0, 1, 1, 1});
            }
            if (HasOutlet(type)) {
                if (IsBranchBlockType(type)) {
                    Rectangle outletRect = {entry->P.x + metrics.outlet.origin.x + entry->hStretch, 
                                            entry->P.y + metrics.outlet.origin.y,
                                            metrics.outlet.size.w,
                                            metrics.outlet.size.h};
                    DEBUGPushRectOutline(outletRect, v4{1, 0, 1, 1});
                }
                else {
                    Rectangle outletRect = {entry->P.x + metrics.outlet.origin.x, 
                                            entry->P.y + metrics.outlet.origin.y,
                                            metrics.outlet.size.w,
                                            metrics.outlet.size.h};
                    DEBUGPushRectOutline(outletRect, v4{1, 0, 1, 1});
                }
            }
            if (HasInnerOutlet(type)) {
                Rectangle innerOutletRect = {entry->P.x + metrics.innerOutlet.origin.x, 
                                             entry->P.y + metrics.innerOutlet.origin.y,
                                             metrics.innerOutlet.size.w,
                                             metrics.innerOutlet.size.h};
                DEBUGPushRectOutline(innerOutletRect, v4{1, 1, 0, 1});
            }
        }
        #endif
        
    }
    drawCall->vertexCount = (u32)(ArenaAt(vertexArena) - start) / VERTEX_SIZE;
}

void BeginBlocks(BlocksInput input) {
    blocksCtx->input = input;
    
    // Clear per-frame memory
    blocksCtx->frame.used = 0;
    
    blocksCtx->hot.type = InteractionType_None;
    blocksCtx->nextHot.type = InteractionType_None;
    
    // Update view metrics
    blocksCtx->screenSize = input.screenSize;
    
    if (input.commandDown) {
        blocksCtx->zoomLevel += input.wheelDelta.y * 0.01;
        blocksCtx->zoomLevel = Clamp(blocksCtx->zoomLevel, 0.25, 16.0);
    }
    else {
        blocksCtx->cameraOrigin.x += input.wheelDelta.x * 0.1;
        blocksCtx->cameraOrigin.y += input.wheelDelta.y * 0.1;
    } 
}

BlocksRenderInfo EndBlocks() {
    if (Interacting()) {
        Interaction *interact = &blocksCtx->interacting;
        
        if (!blocksCtx->input.mouseDown) {
            // End interaction
            switch(interact->type) {
                case InteractionType_BlockSelect: {
                    // @TODO: Eventually, toggle script thread execution
                    break;
                }
                case InteractionType_BlockDrag: {
                    DragInfo dragInfo = blocksCtx->dragInfo;
                    // Drop and combine stacks as necessary
                    if (dragInfo.readyToInsert) {
                        // @NOTE: We *always* keep the script we're adding the dragging blocks to and throw out the dragging script
                        switch(dragInfo.insertionType) {
                            case InsertionType_Before: {
                                Assert(HasOutlet(dragInfo.lastBlock->type));
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
                                    if (IsBranchBlockType(dragInfo.firstBlock->type) && !dragInfo.firstBlock->inner) {
                                        ConnectInner(dragInfo.firstBlock, next);
                                    }
                                    else if (HasOutlet(dragInfo.lastBlock->type)) {
                                        Connect(dragInfo.lastBlock, next);
                                    }
                                    else {
                                        // The last block we're inserting doesn't have an outlet (e.g., forever or stop)
                                        // So we take the rest of the existing stack and turn it into a new script
                                        // 
                                        // @TODO: Better method for placing the new stack
                                        Script *restOfStack = CreateScript(dragInfo.scriptLayout.at);
                                        restOfStack->topBlock = next;
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
                                    if (IsBranchBlockType(dragInfo.firstBlock->type) && !dragInfo.firstBlock->inner) {
                                        ConnectInner(dragInfo.firstBlock, inner);
                                    }
                                    else if (HasOutlet(dragInfo.lastBlock->type)) {
                                        Connect(dragInfo.lastBlock, inner);
                                    }
                                    else {
                                        // The last block we're inserting doesn't have an outlet (e.g., forever or stop)
                                        // So we take the rest of the existing stack and turn it into a new script
                                        // 
                                        // @TODO: Better method for placing the new stack
                                        Script *restOfStack = CreateScript(dragInfo.scriptLayout.at);
                                        restOfStack->topBlock = inner;
                                    }
                                }
                                DeleteScript(dragInfo.script);
                                break;
                            }
                            case InsertionType_Around: {
                                Assert(HasInnerOutlet(dragInfo.firstBlock->type) && !dragInfo.firstBlock->inner);
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
                case InteractionType_BlockSelect: {
                    // If the mouse has moved far enough, start dragging
                    if (DistSq(blocksCtx->blocksRenderGroup.mouseP, interact->mouseStartP) > MIN_DRAG_DIST * MIN_DRAG_DIST) {
                        interact->type = InteractionType_BlockDrag;
                        
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
                case InteractionType_BlockDrag: {
                    Script *script = interact->script;
                    script->P.x = blocksCtx->blocksRenderGroup.mouseP.x - interact->mouseOffset.x;
                    script->P.y = blocksCtx->blocksRenderGroup.mouseP.y - interact->mouseOffset.y;
                    
                    break;
                }
                default: { break; }
            }
        }
    }
    else {
        if (blocksCtx->nextHot.type != InteractionType_None) {
            blocksCtx->hot = blocksCtx->nextHot;
        }
        if (blocksCtx->hot.type != InteractionType_None && blocksCtx->input.mouseDown) {
            // Begin interaction
            if (blocksCtx->hot.type == InteractionType_NewBlockSelect) {
                // Start a dragging interaction with a new block, instead of passing along the existing interaction
                v2 P = blocksCtx->blocksRenderGroup.mouseP;
                Script *script = CreateScript(P);
                Block *block = CreateBlock(BlockType_Command);
                script->topBlock = block;
                
                RenderEntry *entry = PushRenderEntry(&blocksCtx->blocksRenderGroup);
                entry->type = RenderEntryTypeForBlockType(BlockType_Command);
                entry->block = block;
                entry->P = P;
                entry->color = ColorForBlockType(BlockType_Command);
                entry->scale = 1.0f;
                
                Interaction interaction = {};
                interaction.type = InteractionType_BlockDrag;
                interaction.block = block;
                interaction.blockP = P;
                interaction.script = script;
                interaction.entry = entry;
                interaction.mouseStartP = P;
                interaction.mouseOffset = { 0, 0 };
                
                // Set constant dragInfo
                blocksCtx->dragInfo.script = script;
                blocksCtx->dragInfo.firstBlock = block;
                blocksCtx->dragInfo.lastBlock = block;
                
                blocksCtx->interacting = interaction;
            }
            else {
                blocksCtx->interacting = blocksCtx->hot;
            }
        }
    }
    
    // Change the color of the hot block
    if (blocksCtx->hot.type != InteractionType_None) {
        blocksCtx->hot.entry->color = v4{1, 1, 0, 1};
    }
    
    // Assmble vertex buffer
    BlocksRenderInfo Result = {};
    Result.vertexData = ArenaAt(&blocksCtx->frame);
    
    AssembleVertexBuferForRenderGroup(&blocksCtx->frame, &Result, &blocksCtx->blocksRenderGroup);
    AssembleVertexBuferForRenderGroup(&blocksCtx->frame, &Result, &blocksCtx->uiRenderGroup);
    AssembleVertexBuferForRenderGroup(&blocksCtx->frame, &Result, &blocksCtx->dragRenderGroup);
    AssembleVertexBuferForRenderGroup(&blocksCtx->frame, &Result, &blocksCtx->debugRenderGroup);
    AssembleVertexBuferForRenderGroup(&blocksCtx->frame, &Result, &blocksCtx->fontRenderGroup);
    
    Result.vertexDataSize = (u32)(ArenaAt(&blocksCtx->frame) - Result.vertexData);
    return Result;
}

void RenderNewBlockButton(RenderGroup *renderGroup) {
    BlockMetrics metrics = METRICS[BlockType_Command];
    v2 screenSize = blocksCtx->screenSize;
    f32 scale = 4.0f;
    f32 edgePadding = 20.0f;
    v2 P = {screenSize.w - edgePadding - (scale * metrics.size.w),
            screenSize.h - edgePadding - (scale * metrics.size.h)};
    Rectangle hitBox = {P.x, P.y, scale * metrics.size.w, scale * metrics.size.h};
    
    RenderEntry *entry = PushRenderEntry(renderGroup);
    entry->type = RenderEntryType_Command;
    entry->P = P;
    entry->color = COLOR_GREY_50;
    entry->scale = scale;
    
    if (PointInRect(renderGroup->mouseP, hitBox)) {
        // entry->color = v4{1, 1, 0, 1};
        
        blocksCtx->nextHot.type = InteractionType_NewBlockSelect;
        // blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        // blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.entry = entry;
        blocksCtx->nextHot.mouseStartP = renderGroup->mouseP;
        blocksCtx->nextHot.mouseOffset = { renderGroup->mouseP.x - P.x, renderGroup->mouseP.y - P.y };
    }
    
    
}

Layout RenderScript(RenderGroup *renderGroup, Script *script) {
    Assert(script->topBlock);
    
    Layout layout = CreateEmptyLayoutAt(script->P);
    DrawSubScript(renderGroup, script->topBlock, script, &layout);
    return layout;
}

void DrawSubScript(RenderGroup *renderGroup, Block *block, Script *script, Layout *layout) {
    Assert(block);
    // Draw a single linear group of blocks, only recursing on branching blocks
    Rectangle inletBounds = {layout->at.x - 4, layout->at.y, 8, 16};
    Block *nextBlock = block;
    while (nextBlock) {
        if(DrawBlock(renderGroup, nextBlock, script, layout)) {
            nextBlock = nextBlock->next;
        }
        else {
            nextBlock = 0;
        }
    }
    
    // If we're dragging a branch block, check to see if we should place it around another stack
    if (Dragging() && !blocksCtx->dragInfo.readyToInsert) {
        DragInfo dragInfo = blocksCtx->dragInfo;
        if (dragInfo.script != script 
            && !dragInfo.script->topBlock->inner
            && HasInnerOutlet(dragInfo.firstBlock->type)
            && HasInlet(block->type)) {
            if (RectsIntersect(inletBounds, dragInfo.innerOutlet)) {
                Layout loopLayout = CreateEmptyLayoutAt(layout->bounds.origin.x - 6, layout->bounds.origin.y);
                DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, &loopLayout, layout);
                blocksCtx->dragInfo.readyToInsert = true;
                blocksCtx->dragInfo.insertionType = InsertionType_Around;
                blocksCtx->dragInfo.insertionBaseBlock = block;
                blocksCtx->dragInfo.insertionBaseScript = script;
                // DEBUGPushRectOutline(loopLayout.bounds, {0, 1, 0, 1});
            }
        }
    }
    
    #if 0
    // Draw layout bounds
    DEBUGPushRectOutline(layout->bounds, {0, 1, 0, 1});
    #endif
}

b32 DrawBlock(RenderGroup *renderGroup, Block *block, Script *script, Layout *layout) {
    DragInfo dragInfo = blocksCtx->dragInfo;
    
    // Draw ghost block before this block, if necessary
    if (Dragging()
        && dragInfo.script != script
        && IsTopBlockOfScript(block, script)
        && !blocksCtx->dragInfo.readyToInsert 
        && HasInlet(block->type)
        && HasOutlet(dragInfo.lastBlock->type))
    {
        BlockMetrics blockMetrics = METRICS[block->type];
        BlockMetrics dragBlockMetrics = METRICS[dragInfo.lastBlock->type];
        Rectangle inletBounds = TranslateRectangle(blockMetrics.inlet, script->P);
        DEBUGPushRectOutline(inletBounds, COLOR_RED);
        if (RectsIntersect(inletBounds, dragInfo.outlet)) {
            layout->at.x -= dragBlockMetrics.size.w;
            layout->bounds.x -= dragBlockMetrics.size.w;
            DrawGhostBlock(renderGroup, dragInfo.lastBlock->type, layout);
            
            blocksCtx->dragInfo.readyToInsert = true;
            blocksCtx->dragInfo.insertionType = InsertionType_Before;
            blocksCtx->dragInfo.insertionBaseBlock = block;
            blocksCtx->dragInfo.insertionBaseScript = script;
        }
    }
    
    // Draw the block
    if (IsSimpleBlockType(block->type)) {
        DrawSimpleBlock(renderGroup, block->type, block, script, layout);
    }
    else if (IsBranchBlockType(block->type)) {
        BlockMetrics blockMetrics = METRICS[block->type];
        Layout innerLayout = CreateEmptyLayoutAt(layout->at.x + blockMetrics.innerOrigin.x, layout->at.y + blockMetrics.innerOrigin.y);
        b32 renderedInner = false;
        
        // Draw ghost block inside the branch, if necessary
        if (Dragging() 
            && dragInfo.script != script 
            && !blocksCtx->dragInfo.readyToInsert
            && HasInnerOutlet(block->type)
            && HasInlet(dragInfo.firstBlock->type)) 
        {
            Rectangle innerOutletBounds = TranslateRectangle(blockMetrics.innerOutlet, layout->at);
            DEBUGPushRectOutline(innerOutletBounds, COLOR_GREEN);
            if (RectsIntersect(innerOutletBounds, dragInfo.inlet)) {
                if (IsSimpleBlockType(dragInfo.firstBlock->type)) {
                    DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, &innerLayout);
                }
                else if (IsBranchBlockType(block->type)) {
                    // Override block drawing so that branch contains the rest of the substack
                    BlockMetrics dragMetrics = METRICS[dragInfo.firstBlock->type]; // @TODO: Double-check this. Is this the right metrics to be grabbing here?
                    Layout innerInnerLayout = CreateEmptyLayoutAt(innerLayout.at.x + dragMetrics.innerOrigin.x, innerLayout.at.y);
                    if (block->inner && !dragInfo.firstBlock->inner) {
                        blocksCtx->dragInfo.readyToInsert = true; // Set this here so that the inner substack doesn't also try to draw a ghost block
                        DrawSubScript(renderGroup, block->inner, script, &innerInnerLayout);
                        renderedInner = true;
                    }
                    DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, &innerLayout, &innerInnerLayout);
                }
                else {
                    Invalid;
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
            DrawSubScript(renderGroup, block->inner, script, &innerLayout);
        }
        DrawBranchBlock(renderGroup, block->type, block, script, layout, &innerLayout);
    }
    else {
        Invalid;
    }
    
    // Draw ghost block after this block, if necessary
    if (Dragging() 
        && dragInfo.script != script 
        && !blocksCtx->dragInfo.readyToInsert
        && HasOutlet(block->type)
        && HasInlet(dragInfo.firstBlock->type))
    {
        BlockMetrics blockMetrics = METRICS[block->type];
        Rectangle outletBounds = TranslateRectangle(blockMetrics.outlet, {layout->at.x - blockMetrics.size.w, layout->at.y});
        DEBUGPushRectOutline(outletBounds, COLOR_BLUE);
        if (RectsIntersect(outletBounds, dragInfo.inlet)) {
            if (IsSimpleBlockType(dragInfo.firstBlock->type)) {
                DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, layout);
                
                blocksCtx->dragInfo.readyToInsert = true;
                blocksCtx->dragInfo.insertionType = InsertionType_After;
                blocksCtx->dragInfo.insertionBaseBlock = block;
                blocksCtx->dragInfo.insertionBaseScript = script;
            }
            else if (IsBranchBlockType(dragInfo.firstBlock->type)) {
                if (dragInfo.script->topBlock->inner) {
                    // If the loop already contains an inner stack, just put it in line
                    DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, layout);
                    blocksCtx->dragInfo.readyToInsert = true;
                    blocksCtx->dragInfo.insertionType = InsertionType_After;
                    blocksCtx->dragInfo.insertionBaseBlock = block;
                    blocksCtx->dragInfo.insertionBaseScript = script;
                }
                else {
                    // Otherwise, override block drawing so that loop contains the rest of the substack
                    BlockMetrics dragMetrics = METRICS[dragInfo.firstBlock->type];
                    Layout innerLayout = CreateEmptyLayoutAt(layout->at.x + dragMetrics.innerOrigin.x, layout->at.y);
                    blocksCtx->dragInfo.readyToInsert = true; // Set this here so that the inner substack doesn't also try to draw a ghost block
                    if (block->next) {
                        DrawSubScript(renderGroup, block->next, script, &innerLayout);
                    }
                    DrawGhostBlock(renderGroup, dragInfo.firstBlock->type, layout, &innerLayout);
                    blocksCtx->dragInfo.readyToInsert = true;
                    blocksCtx->dragInfo.insertionType = InsertionType_After;
                    blocksCtx->dragInfo.insertionBaseBlock = block;
                    blocksCtx->dragInfo.insertionBaseScript = script;
                    
                    // @TODO: I don't love this weird return boolean thing. Is there a way to avoid this?
                    return false; // Don't continue drawing this substack
                }
            }
            else {
                Invalid;
            }
        }
    }
    
    return true;
}

void DrawSimpleBlock(RenderGroup *renderGroup, BlockType blockType, Block *block, Script *script, Layout *layout, u32 flags) {
    b32 isGhost = flags & DrawBlockFlags_Ghost;
    if (block) {
        Assert(blockType == block->type);
        Assert(IsSimpleBlockType(blockType));
    }
    else {
        Assert(isGhost);
    }
    
    BlockMetrics metrics = METRICS[blockType];
    
    Rectangle hitBox = { layout->at.x, layout->at.y, metrics.size.w, metrics.size.h};
    
    RenderEntry *entry = PushRenderEntry(renderGroup);
    entry->type = RenderEntryTypeForBlockType(blockType);
    entry->block = block;
    entry->P = v2{layout->at.x, layout->at.y};
    if (isGhost) {
        entry->color = v4{1, 1, 1, 0.5};
    }
    else {
        entry->color = ColorForBlockType(blockType);
    }
    entry->scale = 1.0f;
    
    
    layout->at.x += metrics.size.w;

    layout->bounds.w += metrics.size.w;
    layout->bounds.h = Max(layout->bounds.h, metrics.size.h);
    
    if (!isGhost && PointInRect(renderGroup->mouseP, hitBox)) {
        blocksCtx->nextHot.type = InteractionType_BlockSelect;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.entry = entry;
        blocksCtx->nextHot.mouseStartP = renderGroup->mouseP;
        blocksCtx->nextHot.mouseOffset = { renderGroup->mouseP.x - script->P.x, renderGroup->mouseP.y - script->P.y };
    }
}

void DrawBranchBlock(RenderGroup *renderGroup, BlockType blockType, Block *block, Script *script, Layout *layout, Layout *innerLayout, u32 flags) {
    b32 isGhost = flags & DrawBlockFlags_Ghost;
    if (block) {
        Assert(blockType == block->type);
        Assert(IsBranchBlockType(blockType));
    }
    else {
        Assert(isGhost);
    }
    
    BlockMetrics metrics = METRICS[blockType];
    
    u32 horizStretch = 0;
    u32 vertStretch = 0;
    if (innerLayout) {
        horizStretch = Max(innerLayout->bounds.w - metrics.innerSize.w, 0);
        vertStretch = Max(innerLayout->bounds.h - metrics.innerSize.h, 0);
    }
    
        
    Rectangle hitBox = { layout->at.x, layout->at.y, metrics.size.w + (f32)horizStretch, metrics.size.h + (f32)vertStretch };
    Rectangle innerHitBox = { layout->at.x + metrics.innerOrigin.x, layout->at.y, metrics.innerSize.w + (f32)horizStretch, metrics.innerSize.h + (f32)vertStretch };
    
    RenderEntry *entry = PushRenderEntry(renderGroup);
    entry->type = RenderEntryTypeForBlockType(blockType);
    entry->block = block;
    entry->P = v2{layout->at.x, layout->at.y};
    if (isGhost) {
        entry->color = v4{1, 1, 1, 0.5};
    }
    else {
        entry->color = ColorForBlockType(blockType);
    }
    entry->hStretch = horizStretch;
    entry->vStretch = vertStretch;
    
    layout->at.x += metrics.size.w + horizStretch;
    
    layout->bounds.w += metrics.size.w + horizStretch;
    layout->bounds.h = Max(layout->bounds.h, metrics.size.h + vertStretch);
    
    if (!isGhost && PointInRect(renderGroup->mouseP, hitBox) && !PointInRect(renderGroup->mouseP, innerHitBox)) {
        blocksCtx->nextHot.type = InteractionType_BlockSelect;
        blocksCtx->nextHot.block = block;
        blocksCtx->nextHot.blockP = entry->P;
        blocksCtx->nextHot.script = script;
        blocksCtx->nextHot.entry = entry;
        blocksCtx->nextHot.mouseStartP = renderGroup->mouseP;
        blocksCtx->nextHot.mouseOffset = { renderGroup->mouseP.x - script->P.x, renderGroup->mouseP.y - script->P.y };
    }
}

void DrawGhostBlock(RenderGroup *renderGroup, BlockType blockType, Layout *layout, Layout *innerLayout) {
    if (IsSimpleBlockType(blockType)) {
        DrawSimpleBlock(renderGroup, blockType, NULL, NULL, layout, DrawBlockFlags_Ghost);
    }
    else if (IsBranchBlockType(blockType)) {
        DrawBranchBlock(renderGroup, blockType, NULL, NULL, layout, innerLayout, DrawBlockFlags_Ghost);
    }
    else {
        Invalid;
    }
}


extern "C" void InitBlocks(void *mem, u32 memSize) {
    static const u32 VERTS_MEM_SIZE = 65535 * (8 * sizeof(f32));
    
    Assert(memSize >= sizeof(BlocksContext) + VERTS_MEM_SIZE);
    
    Arena dummyArena = {};
    dummyArena.data = (u8 *)mem;
    dummyArena.size = memSize;
    dummyArena.used = 0;
    
    BlocksContext *context = PushStruct(&dummyArena, BlocksContext);
    u32 permanentArenaSize = dummyArena.size - dummyArena.used - VERTS_MEM_SIZE;
    
    context->permanent = SubArena(&dummyArena, permanentArenaSize);
    context->frame = SubArena(&dummyArena, VERTS_MEM_SIZE);
    
    context->scriptCount = 0;
    
    context->zoomLevel = 3.0f;
    context->cameraOrigin = v2{0, 0};
    
    blocksCtx = context;
    
    // Create some blocks, y'know, for fun
    {
        // A script with a single command block
        Script *script = CreateScript(v2{0, 0});
        Block *block = CreateBlock(BlockType_Command);
        script->topBlock = block;
    }
    
    {
        // A script with some different types of blocks
        Script *script = CreateScript(v2{-40, -25});
        Block *event = CreateBlock(BlockType_Event);
        script->topBlock = event;
        
        Block *command = CreateBlock(BlockType_Command);
        Connect(event, command);
        
        Block *repeat = CreateBlock(BlockType_Loop);
        Connect(command, repeat);
        
        Block *forever = CreateBlock(BlockType_Forever);
        Connect(repeat, forever);
    }
    
    {
        // Same as previous, but with an end cap instead of a forever at the end
        Script *script = CreateScript(v2{-40, -50});
        Block *event = CreateBlock(BlockType_Event);
        script->topBlock = event;
        
        Block *command = CreateBlock(BlockType_Command);
        Connect(event, command);
        
        Block *repeat = CreateBlock(BlockType_Loop);
        Connect(command, repeat);
        
        Block *endCap = CreateBlock(BlockType_EndCap);
        Connect(repeat, endCap);
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
    
    TransformPair blocksTransformPair = BlocksCameraTransformPair(blocksCtx->screenSize, blocksCtx->zoomLevel, blocksCtx->cameraOrigin);
    
    InitRenderGroup(&blocksCtx->debugRenderGroup, blocksTransformPair.transform, blocksTransformPair.invTransform);
    
    RenderGroup *blocksRenderGroup = &blocksCtx->blocksRenderGroup;
    InitRenderGroup(blocksRenderGroup, blocksTransformPair.transform, blocksTransformPair.invTransform);
    
    RenderGroup *dragRenderGroup = &blocksCtx->dragRenderGroup;
    InitRenderGroup(dragRenderGroup, blocksTransformPair.transform, blocksTransformPair.invTransform);
    
    RenderGroup *fontRenderGroup = &blocksCtx->fontRenderGroup;
    InitRenderGroup(fontRenderGroup, blocksTransformPair.transform, blocksTransformPair.invTransform);
    
    if (Dragging()) {
        // Update dragging info
        Script *script = blocksCtx->dragInfo.script;
        Block *firstBlock = blocksCtx->dragInfo.firstBlock;
        Block *lastBlock = blocksCtx->dragInfo.lastBlock;
        BlockMetrics firstMetrics = METRICS[firstBlock->type];
        BlockMetrics lastMetrics = METRICS[lastBlock->type];
        
        Layout dragLayout = RenderScript(dragRenderGroup, script);
        blocksCtx->dragInfo.scriptLayout = dragLayout;
        
        DEBUGPushRectOutline(dragLayout.bounds, COLOR_GREEN);
        
        if (HasInlet(firstBlock->type)) {
            blocksCtx->dragInfo.inlet = TranslateRectangle(firstMetrics.inlet, dragLayout.bounds.origin);
            DEBUGPushRectOutline(blocksCtx->dragInfo.inlet, COLOR_CYAN);
        }
        if (HasOutlet(lastBlock->type)) {
            // Account for outlet offset in block metrics
            blocksCtx->dragInfo.outlet = TranslateRectangle(lastMetrics.outlet, {dragLayout.at.x - lastMetrics.size.w, dragLayout.at.y});
            DEBUGPushRectOutline(blocksCtx->dragInfo.outlet, COLOR_MAGENTA);
        }
        if (HasInnerOutlet(firstBlock->type)) {
            blocksCtx->dragInfo.innerOutlet = TranslateRectangle(firstMetrics.innerOutlet, dragLayout.bounds.origin);
            DEBUGPushRectOutline(blocksCtx->dragInfo.innerOutlet, COLOR_YELLOW);
        }
        
        // Reset this to false each frame so we can update the ghost block insertion point, if necessary
        blocksCtx->dragInfo.readyToInsert = false;
    }
    for (u32 i = 0; i < blocksCtx->scriptCount; ++i) {
        Script *script = &blocksCtx->scripts[i];
        if (Dragging() && blocksCtx->dragInfo.script == script) {
            continue;
        }
        RenderScript(blocksRenderGroup, script);
    }
    
    // Floating UI
    RenderGroup *overlayRenderGroup = &blocksCtx->uiRenderGroup;
    TransformPair oneToOneTransformPair = OneToOneCameraTransformPair(blocksCtx->screenSize);
    InitRenderGroup(overlayRenderGroup, oneToOneTransformPair.transform, oneToOneTransformPair.invTransform);
    RenderNewBlockButton(overlayRenderGroup);
    
    return EndBlocks();
}
