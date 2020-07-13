/*********************************************************
*
* BlocksInternal.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

struct Block;
struct Script;
struct Layout;

enum BlockType {
    BlockType_Command = 0,
    BlockType_Event,
    BlockType_EndCap,
    BlockType_Loop,
    BlockType_Forever,
    
    BlockTypeCount
};

struct Arena {
    u8 *data;
    u32 size;
    u32 used;
};

struct Rectangle {
    union {
        struct {
            f32 x;
            f32 y;
        };
        v2 origin;
    };
    union {
        struct {
            f32 w;
            f32 h;
        };
        v2 size;
    };
};

struct Layout {
    v2 at;
    Rectangle bounds;
};

enum RenderEntryType {
    RenderEntryType_Command,
    RenderEntryType_Event,
    RenderEntryType_EndCap,
    RenderEntryType_Loop,
    RenderEntryType_Forever,
    
    RenderEntryType_Rect,
    RenderEntryType_RectOutline,
    
    RenderEntryType_Text,
    
    RenderEntryType_Null,
};

struct RenderEntry {
    RenderEntryType type;
    Block* block;
    v2 P;
    v4 color;
    f32 scale;
    union {
        struct {
            u32 hStretch;
            u32 vStretch;
        };
    };
    Rectangle rect;
    
    char *text;
    f32 textHeight;
};

enum DrawBlockFlags {
    DrawBlockFlags_None = 0,
    DrawBlockFlags_Ghost = 1 << 0,
};

enum InteractionType {
    InteractionType_None,
    
    InteractionType_BlockSelect,
    InteractionType_BlockDrag,
    
    InteractionType_NewBlockSelect,
};

struct Interaction {
    InteractionType type;
    Block *block;
    v2 blockP;
    Script *script;
    RenderEntry *entry;
    
    v2 mouseStartP;
    v2 mouseOffset;
};

struct Block {
    Block *prev;
    Block *next;
    BlockType type;
    
    // Loops
    Block *inner;
    Block *parent; // Points to the loop block that encloses this sub-stack
};

struct Script {
    v2 P;
    Block *topBlock;
};

enum InsertionType {
    InsertionType_Before,
    InsertionType_After,
    InsertionType_Inside,
    InsertionType_Around,
};

struct DragInfo {
    Script *script;
    Layout scriptLayout;
    Block *firstBlock;
    Block *lastBlock;
    Rectangle inlet;
    Rectangle outlet;
    Rectangle innerOutlet;
    
    b32 readyToInsert;
    InsertionType insertionType;
    Block *insertionBaseBlock;
    Script *insertionBaseScript;
};

struct TransformPair {
    mat4x4 transform;
    mat4x4 invTransform;
};

struct RenderGroup {
    RenderEntry entries[4096];
    u32 entryCount;
    mat4x4 transform;
    mat4x4 invTransform;
    v2 mouseP; // Unprojected into the coordinate system of the render group
};

struct BlocksContext {
    BlocksInput input;
    
    Arena permanent;
    Arena frame;
    
    RenderGroup blocksRenderGroup;
    RenderGroup uiRenderGroup;
    RenderGroup dragRenderGroup;
    RenderGroup debugRenderGroup;
    
    RenderGroup fontRenderGroup;
    
    Script scripts[1024];
    u32 scriptCount;
    
    Interaction hot;
    Interaction interacting;
    Interaction nextHot;
    
    DragInfo dragInfo;
    
    v2 screenSize;
    f32 zoomLevel;
    v2 cameraOrigin;
};

void BeginBlocks(BlocksInput input);
BlocksRenderInfo EndBlocks();
void DrawSubScript(RenderGroup *renderGroup, Block *block, Script *script, Layout *layout);
b32 DrawBlock(RenderGroup *renderGroup, Block *block, Script *script, Layout *layout);
void DrawSimpleBlock(RenderGroup *renderGroup, BlockType blockType, Block *block, Script *script, Layout *layout, u32 flags = 0);
void DrawBranchBlock(RenderGroup *renderGroup, BlockType blockType, Block *block, Script *script, Layout *layout, Layout *innerLayout, u32 flags = 0);
void DrawGhostBlock(RenderGroup *renderGroup, BlockType blockType, Layout *layout, Layout *innerLayout = 0);

void *PushSize(Arena *arena, u32 size) {
  // Make sure we have enough space left in the arena
  Assert(arena->used + size <= arena->size);
  
  void *result = arena->data + arena->used;
  arena->used += size;
  
  return result;
}

void PushData_(Arena *arena, void *data, u32 size) {
    void *location = PushSize(arena, size);
    memcpy(location, data, size);
}

Arena SubArena(Arena *arena, u32 size) {
    Arena subArena = {};
    subArena.data = (u8 *)PushSize(arena, size);
    subArena.size = size;
    subArena.used = 0;
    return subArena;
}

inline
u8 *ArenaAt(Arena *arena) {
    return arena->data + arena->used;
}

#define PushStruct(arena, type) (type *)PushSize(arena, sizeof(type))
#define PushArray(arena, type, count) (type *)PushSize(arena, sizeof(type) * count)
