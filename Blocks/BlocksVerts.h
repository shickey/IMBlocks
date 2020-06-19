
void PushCommandBlockVerts(V2 position, V3 color) {
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
    
    pushVerts(verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

void PushLoopBlockVerts(V2 position, V3 color, u32 stretchUnits = 0) {
    #define unitSize 12.0f
    #define texSize  512.0f
    #define originX  16.0f
    #define originY  448.0f
    
    f32 verts[] = {
        /* Left side (with connectors) */
        -1 + position.x, -1 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
         9 + position.x, -1 + position.y, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 13 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         9 + position.x, -1 + position.y, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 13 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
         9 + position.x, 13 + position.y, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable vertical on left side */
        -1 + position.x, 13 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 13 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 15 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         7 + position.x, 13 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 15 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 15 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Upper left corner */
        -1 + position.x, 15 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 15 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 21 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
         7 + position.x, 15 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        -1 + position.x, 21 + position.y, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 21 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable horizontal on top */
         7 + position.x, 15 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 15 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 21 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        21 + position.x + stretchUnits, 15 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
         7 + position.x, 21 + position.y, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 21 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Upper right corner */
        21 + position.x + stretchUnits, 15 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + stretchUnits, 15 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 21 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        39 + position.x + stretchUnits, 15 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 21 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + stretchUnits, 21 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Stretchable vertical on right side */
        21 + position.x + stretchUnits, 13 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + stretchUnits, 13 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 15 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        39 + position.x + stretchUnits, 13 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 15 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        39 + position.x + stretchUnits, 15 + position.y, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        /* Right side (with connectors) */
        21 + position.x + stretchUnits, -1 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        41 + position.x + stretchUnits, -1 + position.y, (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 13 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        
        41 + position.x + stretchUnits, -1 + position.y, (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)), color.r, color.g, color.b,
        21 + position.x + stretchUnits, 13 + position.y, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b,
        41 + position.x + stretchUnits, 13 + position.y, (((41 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)), color.r, color.g, color.b
    };
    
    pushVerts(verts);
    
    #undef unitSize
    #undef texSize
    #undef originX
    #undef originY
}

#define COMMAND_BLOCK_VERTS(unitSize, texSize, originX, originY) \
    /* X, Y, U, V */\
    -1, -1, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    19, -1, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    -1, 17, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)),\
    \
    19, -1, (((19 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    -1, 17, (((-1 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize)),\
    19, 17, (((19 * unitSize) + originX) / (texSize)), ((originY - (17 * unitSize)) / (texSize))

#define LOOP_BLOCK_VERTS(unitSize, texSize, originX, originY) \
    /* X, Y, U, V */\
    /* Left side (with connectors) */\
   -1,  -1, (((-1 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    9,  -1, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
   -1,  13, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    \
    9,  -1, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
   -1,  13, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    9,  13, ((( 9 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    \
    /* Stretchable vertical on left side */\
   -1,  13, (((-1 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    7,  13, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
   -1,  15, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    \
    7,  13, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
   -1,  15, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    7,  15, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    \
    /* Upper left corner */\
   -1,  15, (((-1 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    7,  15, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
   -1,  21, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    7,  15, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
   -1,  21, (((-1 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    7,  21, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    /* Stretchable horizontal on top */\
    7,  15, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    21, 15, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    7,  21, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    21, 15, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    7,  21, ((( 7 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    21, 21, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    /* Upper right corner */\
    21, 15, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    39, 15, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    21, 21, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    39, 15, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    21, 21, (((21 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    39, 21, (((39 * unitSize) + originX) / (texSize)), ((originY - (21 * unitSize)) / (texSize)),\
    \
    /* Stretchable vertical on right side */\
    21, 13, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    39, 13, (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    21, 15, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    \
    39, 13, (((39 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    21, 15, (((21 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    39, 15, (((39 * unitSize) + originX) / (texSize)), ((originY - (15 * unitSize)) / (texSize)),\
    \
    /* Right side (with connectors) */\
    21, -1, (((21 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    41, -1, (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    21, 13, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    \
    41, -1, (((41 * unitSize) + originX) / (texSize)), ((originY - (-1 * unitSize)) / (texSize)),\
    21, 13, (((21 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize)),\
    41, 13, (((41 * unitSize) + originX) / (texSize)), ((originY - (13 * unitSize)) / (texSize))

// global_var f32 CommandBlockVerts[] = { COMMAND_BLOCK_VERTS(8.0, 512.0, 32.0, 160.0) };
//global_var f32 LoopBlockVerts[] = { LOOP_BLOCK_VERTS(12.0, 512.0, 16.0, 444.0) };



// global_var f32 CommandBlockVerts[] = {
// //  X    Y                U               V
//     0,   0,  (32.0 / 512.0), (160.0 / 512.0), 
//     18,  0, (176.0 / 512.0), (160.0 / 512.0),
//     0,  16,  (32.0 / 512.0),  (32.0 / 512.0),
    
//     18,  0, (176.0 / 512.0), (160.0 / 512.0),
//     0,  16,  (32.0 / 512.0),  (32.0 / 512.0),
//     18, 16, (176.0 / 512.0),  (32.0 / 512.0)
// };


// global_var f32 LoopBlockVerts[] = {
// //  X    Y                U               V
    
//     // Left side (with connectors)
//    -1,  -1,  (24.0 / 512.0), (360.0 / 512.0),
//     9,  -1, (104.0 / 512.0), (360.0 / 512.0),
//    -1,  13,  (24.0 / 512.0), (248.0 / 512.0),
    
//     9,  -1, (104.0 / 512.0), (360.0 / 512.0),
//    -1,  13,  (24.0 / 512.0), (248.0 / 512.0),
//     9,  13, (104.0 / 512.0), (248.0 / 512.0),
    
//     // Stretchable vertical on left side
//    -1,  13,  (24.0 / 512.0), (248.0 / 512.0),
//     7,  13,  (88.0 / 512.0), (248.0 / 512.0),
//    -1,  15,  (24.0 / 512.0), (232.0 / 512.0),
    
//     7,  13,  (88.0 / 512.0), (248.0 / 512.0),
//    -1,  15,  (24.0 / 512.0), (232.0 / 512.0),
//     7,  15,  (88.0 / 512.0), (232.0 / 512.0),
    
//     // Upper left corner
//    -1,  15,  (24.0 / 512.0), (232.0 / 512.0),
//     7,  15,  (88.0 / 512.0), (232.0 / 512.0),
//    -1,  21,  (24.0 / 512.0), (184.0 / 512.0),
    
//     7,  15,  (88.0 / 512.0), (232.0 / 512.0),
//    -1,  21,  (24.0 / 512.0), (184.0 / 512.0),
//     7,  21,  (88.0 / 512.0), (184.0 / 512.0),
    
//     // Stretchable horizontal on top
//     7,  15,  (88.0 / 512.0), (232.0 / 512.0),
//     21, 15, (200.0 / 512.0), (232.0 / 512.0),
//     7,  21,  (88.0 / 512.0), (184.0 / 512.0),
    
//     21, 15, (200.0 / 512.0), (232.0 / 512.0),
//     7,  21,  (88.0 / 512.0), (184.0 / 512.0),
//     21, 21, (200.0 / 512.0), (184.0 / 512.0),
    
//     // Upper right corner
//     21, 15, (200.0 / 512.0), (232.0 / 512.0),
//     39, 15, (344.0 / 512.0), (232.0 / 512.0),
//     21, 21, (200.0 / 512.0), (184.0 / 512.0),
    
//     39, 15, (344.0 / 512.0), (232.0 / 512.0),
//     21, 21, (200.0 / 512.0), (184.0 / 512.0),
//     39, 21, (344.0 / 512.0), (184.0 / 512.0),
    
//     // Stretchable vertical on left side
//     21, 13, (200.0 / 512.0), (248.0 / 512.0),
//     39, 13, (344.0 / 512.0), (248.0 / 512.0),
//     21, 15, (200.0 / 512.0), (232.0 / 512.0),
    
//     39, 13, (344.0 / 512.0), (248.0 / 512.0),
//     21, 15, (200.0 / 512.0), (232.0 / 512.0),
//     39, 15, (344.0 / 512.0), (232.0 / 512.0),
    
//     // Right side (with connectors)
//     21,  -1, (200.0 / 512.0), (360.0 / 512.0),
//     41,  -1, (360.0 / 512.0), (360.0 / 512.0),
//     21,  13, (200.0 / 512.0), (248.0 / 512.0),
    
//     41,  -1, (360.0 / 512.0), (360.0 / 512.0),
//     21,  13, (200.0 / 512.0), (248.0 / 512.0),
//     41,  13, (360.0 / 512.0), (248.0 / 512.0)
    
// };
