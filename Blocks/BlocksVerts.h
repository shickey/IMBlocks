/*********************************************************
*
* BlocksVerts.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#define PushVerts(ctx, v) PushData_(&ctx->verts, (v), sizeof((v)))
#define VERTEX_SIZE (8 * sizeof(f32))

void PushRect(BlocksContext *ctx, Rectangle rect, v4 color) {
    
    // @TODO: @NOTE: The UV coords here are just a silly hack. They point at a texel firmly inside one of the block shapes (to force shader to definitely draw the fragments)
    f32 verts[] = {
        // Lower tri
        rect.x,          rect.y,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w, rect.y,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x,          rect.y + rect.h, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Upper tri
        rect.x + rect.w,  rect.y,         100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x,          rect.y + rect.h, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w, rect.y + rect.h, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
    };
    
    PushVerts(ctx, verts);
}

void PushRectOutline(BlocksContext *ctx, Rectangle rect, v4 color) {
    #define rectWidth 0.5f
    #define rectHalfWidth (rectWidth / 2.0f)
    
    // @TODO: @NOTE: The UV coords here are just a silly hack. They point at a texel firmly inside one of the block shapes (to force shader to definitely draw the fragments)
    f32 verts[] = {
        // Bottom
        rect.x - rectHalfWidth,          rect.y - rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y - rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y - rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Top
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h + rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h + rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h + rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Left
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rectHalfWidth,          rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rectHalfWidth,          rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Right
        rect.x + rect.w - rectHalfWidth, rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w - rectHalfWidth, rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w - rectHalfWidth, rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 100.0f / 512.0f, 100.0f / 512.0f, color.r, color.g, color.b, color.a,
    };
    
    PushVerts(ctx, verts);
    
    #undef rectHalfWidth
    #undef rectWidth
}

void PushCommandBlockVerts(BlocksContext *ctx, v2 position, v4 color) {
    #define unitSize 8.0f
    #define texSize  512.0f
    #define originX  32.0f
    #define originY  160.0f
    
    f32 verts[] = {
        -1 + position.x, -1 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        19 + position.x, -1 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        19 + position.x, -1 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        19 + position.x, 17 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(ctx, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushLoopBlockVerts(BlocksContext *ctx, v2 position, v4 color, u32 horizontalStretch = 0, u32 verticalStretch = 0) {
    #define unitSize 12.0f
    #define texSize  512.0f
    #define originX  16.0f
    #define originY  448.0f
    
    f32 verts[] = {
        /* Left side (with connectors) */
        -1 + position.x,                     -1 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         9 + position.x,                     -1 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
         9 + position.x,                     -1 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         9 + position.x,                     13 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Stretchable vertical on left side */
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     13 + position.y,                   ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
         7 + position.x,                     13 + position.y,                   ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Upper left corner */
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     21 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x,                     21 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Stretchable horizontal on top */
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Upper right corner */
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        39 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Stretchable vertical on right side */
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        39 + position.x + horizontalStretch, 13 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        39 + position.x + horizontalStretch, 13 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        /* Right side (with connectors) */
        21 + position.x + horizontalStretch, -1 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        41 + position.x + horizontalStretch, -1 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        41 + position.x + horizontalStretch, -1 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        41 + position.x + horizontalStretch, 13 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(ctx, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

