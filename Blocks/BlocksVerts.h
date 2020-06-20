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
#define VERTEX_SIZE (7 * sizeof(f32))

void PushCommandBlockVerts(BlocksContext *ctx, v2 position, v3 color) {
    #define unitSize 8.0f
    #define texSize  512.0f
    #define originX  32.0f
    #define originY  160.0f
    
    f32 verts[] = {
        -1 + position.x, -1 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        19 + position.x, -1 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        19 + position.x, -1 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b,
        19 + position.x, 17 + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b
    };
    
    PushVerts(ctx, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushLoopBlockVerts(BlocksContext *ctx, v2 position, v3 color, u32 horizontalStretch = 0, u32 verticalStretch = 0) {
    #define unitSize 12.0f
    #define texSize  512.0f
    #define originX  16.0f
    #define originY  448.0f
    
    f32 verts[] = {
        /* Left side (with connectors) */
        -1 + position.x,                     -1 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
         9 + position.x,                     -1 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         9 + position.x,                     -1 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
         9 + position.x,                     13 + position.y,                   ((( 9 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable vertical on left side */
        -1 + position.x,                     13 + position.y,                   (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     13 + position.y,                   ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         7 + position.x,                     13 + position.y,                   ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Upper left corner */
        -1 + position.x,                     15 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     21 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x,                     21 + position.y + verticalStretch, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable horizontal on top */
         7 + position.x,                     15 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x,                     21 + position.y + verticalStretch, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Upper right corner */
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + horizontalStretch, 21 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable vertical on right side */
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + horizontalStretch, 13 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        39 + position.x + horizontalStretch, 13 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + horizontalStretch, 15 + position.y + verticalStretch, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Right side (with connectors) */
        21 + position.x + horizontalStretch, -1 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        41 + position.x + horizontalStretch, -1 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        41 + position.x + horizontalStretch, -1 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        41 + position.x + horizontalStretch, 13 + position.y,                   (((41 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b
    };
    
    PushVerts(ctx, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

