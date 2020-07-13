/*********************************************************
*
* BlocksVerts.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#include "font-atlas.h"

#define PushVerts(arena, v) PushData_(arena, (v), sizeof((v)))
#define VERTEX_SIZE (8 * sizeof(f32))

#define COLOR_RED     v4{1, 0, 0, 1}
#define COLOR_GREEN   v4{0, 1, 0, 1}
#define COLOR_BLUE    v4{0, 0, 1, 1}
#define COLOR_CYAN    v4{0, 1, 1, 1}
#define COLOR_MAGENTA v4{1, 0, 1, 1}
#define COLOR_YELLOW  v4{1, 1, 0, 1}
#define COLOR_BLACK   v4{0, 0, 0, 1}
#define COLOR_WHITE   v4{1, 1, 1, 1}
#define COLOR_GREY_25 v4{0.25, 0.25, 0.25, 1}
#define COLOR_GREY_50 v4{0.50, 0.50, 0.50, 1}
#define COLOR_GREY_75 v4{0.75, 0.75, 0.75, 1}

struct BlockMetrics {
    v2 size;
    v2 sizeWithConnectors;
    
    // Everything below here is relative to the block origin (lower-left corner)
    v2 innerOrigin;
    v2 innerSize;
    
    Rectangle inlet;
    Rectangle outlet;
    Rectangle innerOutlet;
};

global_var BlockMetrics METRICS[BlockTypeCount] = {
    { // BlockType_Command
        v2{16, 16},
        v2{18, 16},
        v2{0, 0},
        v2{0, 0},
        Rectangle{-4, 0, 8, 16},
        Rectangle{12, 0, 8, 16},
        Rectangle{0, 0, 0, 0},
    },
    { // BlockType_Event
        v2{17, 16},
        v2{19, 16},
        v2{0, 0},
        v2{0, 0},
        Rectangle{0, 0, 0, 0},
        Rectangle{13, 0, 8, 16},
        Rectangle{0, 0, 0, 0},
    },
    { // BlockType_EndCap
        v2{17, 16},
        v2{17, 16},
        v2{0, 0},
        v2{0, 0},
        Rectangle{-4, 0, 8, 16},
        Rectangle{0, 0, 0, 0},
        Rectangle{0, 0, 0, 0},
    },
    { // BlockType_Loop
        v2{38, 20},
        v2{40, 20},
        v2{6, 0},
        v2{16, 16},
        Rectangle{-4, 0, 7, 16},
        Rectangle{34, 0, 8, 16},
        Rectangle{3, 0, 6, 16},
    },
    { // BlockType_Forever
        v2{38, 20},
        v2{38, 20},
        v2{6, 0},
        v2{16, 16},
        Rectangle{-4, 0, 7, 16},
        Rectangle{0, 0, 0, 0},
        Rectangle{3, 0, 6, 16},
    },
};

void PushRect(Arena *arena, Rectangle rect, v2 uv0, v2 uv1, v4 color) {
    
    f32 verts[] = {
        // Lower tri
        rect.x,          rect.y,          uv0.u, uv0.v, color.r, color.g, color.b, color.a,
        rect.x + rect.w, rect.y,          uv1.u, uv0.v, color.r, color.g, color.b, color.a,
        rect.x,          rect.y + rect.h, uv0.u, uv1.v, color.r, color.g, color.b, color.a,
        
        // Upper tri
        rect.x + rect.w, rect.y,          uv1.u, uv0.v, color.r, color.g, color.b, color.a,
        rect.x,          rect.y + rect.h, uv0.u, uv1.v, color.r, color.g, color.b, color.a,
        rect.x + rect.w, rect.y + rect.h, uv1.u, uv1.v, color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
}

void PushSolidRect(Arena *arena, Rectangle rect, v4 color) {
    // @TODO: @NOTE: The UV coords here are just a silly hack. They point at a texel firmly inside one of the block shapes (to force shader to definitely draw the fragments)
    v2 uv = v2{75.0f / 255.0f, 75.0f / 255.0f};
    PushRect(arena, rect, uv, uv, color);
}

void PushChar(Arena *arena, SdfFontChar character, f32 fontScale, v2 at, v4 color) {
    Rectangle rect = Rectangle{ at.x + (fontScale * character.xOffset), 
                                at.y - (fontScale * (character.h + character.yOffset)), 
                                (f32)character.w * fontScale, 
                                (f32)character.h * fontScale };
    v2 uv0 = v2{ (f32)character.x0 / 512.0f, (f32)character.y1 / 512.0f }; // Flip y
    v2 uv1 = v2{ (f32)character.x1 / 512.0f, (f32)character.y0 / 512.0f };
    PushRect(arena, rect, uv0, uv1, color);
}

// @TODO: Write our own strlen
void PushFontString(Arena *arena, const char *str, v2 at, f32 fontHeight, v4 color) {
    for (u32 i = 0; i < strlen(str); ++i) {
        SdfFontChar c = FONT_DATA[str[i]];
        f32 fontScale = ScaleForFontHeight(fontHeight);
        PushChar(arena, c, fontScale, at, color);
        at.x += c.advance * fontScale;
        if (i < strlen(str) - 1) {
            f32 kern = KERN_TABLE[str[i + 1]][str[i]];
            at.x += kern * fontScale;
        }
    }
}

void PushRectOutline(Arena *arena, Rectangle rect, v4 color) {
    #define rectWidth 0.5f
    #define rectHalfWidth (rectWidth / 2.0f)
    
    // @TODO: @NOTE: The UV coords here are just a silly hack. They point at a texel firmly inside one of the block shapes (to force shader to definitely draw the fragments)
    f32 verts[] = {
        // Bottom
        rect.x - rectHalfWidth,          rect.y - rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y - rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y - rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Top
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h + rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h + rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h + rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Left
        rect.x - rectHalfWidth,          rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rectHalfWidth,          rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rectHalfWidth,          rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x - rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rectHalfWidth,          rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        // Right
        rect.x + rect.w - rectHalfWidth, rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w - rectHalfWidth, rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        
        rect.x + rect.w + rectHalfWidth, rect.y + rectHalfWidth,          75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w - rectHalfWidth, rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
        rect.x + rect.w + rectHalfWidth, rect.y + rect.h - rectHalfWidth, 75.0f / 512.0f, 75.0f / 512.0f, color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
    
    #undef rectHalfWidth
    #undef rectWidth
}

void PushCommandBlockVerts(Arena *arena, v2 position, v4 color, f32 scale = 1) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  32.0f
    #define originY  96.0f
    
    f32 verts[] = {
        (-1 * scale) + position.x, (-1 * scale) + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        (19 * scale) + position.x, (-1 * scale) + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        (-1 * scale) + position.x, (17 * scale) + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        (19 * scale) + position.x, (-1 * scale) + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        (-1 * scale) + position.x, (17 * scale) + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        (19 * scale) + position.x, (17 * scale) + position.y, (((19 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushEndCapBlockVerts(Arena *arena, v2 position, v4 color) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  288.0f
    #define originY  96.0f
    
    f32 verts[] = {
        -1 + position.x, -1 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        18 + position.x, -1 + position.y, (((18 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        18 + position.x, -1 + position.y, (((18 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        18 + position.x, 17 + position.y, (((18 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushEventBlockVerts(Arena *arena, v2 position, v4 color) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  160.0f
    #define originY  96.0f
    
    f32 verts[] = {
        -1 + position.x, -1 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        20 + position.x, -1 + position.y, (((20 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        20 + position.x, -1 + position.y, (((20 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        -1 + position.x, 17 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        20 + position.x, 17 + position.y, (((20 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushNumberInputVerts(Arena *arena, v2 position, v4 color) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  416.0f
    #define originY  128.0f
    
    Rectangle inputRect = Rectangle{ position.x - 1, position.y - 1, 12 + 2, 8 + 2 };
    v2 uv0 = v2{ (originX - (1 * unitSize)) / texSize, (originY + (1 * unitSize)) / texSize };
    v2 uv1 = v2{ (originX + ((inputRect.w - 1) * unitSize)) / texSize, (originY - ((inputRect.h - 1) * unitSize)) / texSize };
    PushRect(arena, inputRect, uv0, uv1, color);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushStringInputVerts(Arena *arena, v2 position, v4 color) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  416.0f
    #define originY  64.0f
    
    Rectangle inputRect = Rectangle{ position.x - 1, position.y - 1, 12 + 2, 8 + 2 };
    v2 uv0 = v2{ (originX - (1 * unitSize)) / texSize, (originY + (1 * unitSize)) / texSize };
    v2 uv1 = v2{ (originX + ((inputRect.w - 1) * unitSize)) / texSize, (originY - ((inputRect.h - 1) * unitSize)) / texSize };
    PushRect(arena, inputRect, uv0, uv1, color);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushLoopBlockVerts(Arena *arena, v2 position, v4 color, u32 horizontalStretch = 0, u32 verticalStretch = 0) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  32.0f
    #define originY  208.0f
    
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
    
    PushVerts(arena, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushForeverBlockVerts(Arena *arena, v2 position, v4 color, u32 horizontalStretch = 0, u32 verticalStretch = 0) {
    #define unitSize 4.0f
    #define texSize  512.0f
    #define originX  232.0f
    #define originY  208.0f
    
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
        39 + position.x + horizontalStretch, -1 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        
        39 + position.x + horizontalStretch, -1 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        21 + position.x + horizontalStretch, 13 + position.y,                   (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
        39 + position.x + horizontalStretch, 13 + position.y,                   (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b, color.a,
    };
    
    PushVerts(arena, verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}
