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

typedef u32 RenderingIndex;

enum BlockType {
    BlockType_Command,
    BlockType_Loop
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
    RenderEntryType_Loop
};

struct RenderEntry {
    RenderingIndex idx;
    RenderEntryType type;
    Block* block;
    v2 P;
    v3 color;
    union {
        struct {
            u32 hStretch;
            u32 vStretch;
        };
    };
};

enum DrawBlockFlags {
    DrawBlockFlags_None = 0,
    DrawBlockFlags_Ghost = 1 << 0,
};

enum InteractionType {
    InteractionType_None,
    InteractionType_Select,
    InteractionType_Drag
};

struct Interaction {
    InteractionType type;
    Block *block;
    v2 blockP;
    Script *script;
    RenderingIndex renderingIdx;
    
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

struct BlocksContext {
    BlocksInput input;
    
    Arena verts;
    Arena blocks;
    
    RenderEntry renderEntries[4096];
    RenderingIndex nextRenderingIdx;
    
    Script scripts[1024];
    u32 scriptCount;
    
    Interaction hot;
    Interaction interacting;
    Interaction nextHot;
    
    DragInfo dragInfo;
};

void BeginBlocks(BlocksInput input);
BlocksRenderInfo EndBlocks(void);
void DrawSubScript(Block *block, Script *script, Layout *layout);
b32 DrawBlock(Block *, Script *, Layout *);
void DrawCommandBlock(Block *, Script *, Layout *, u32 flags = 0);
void DrawLoopBlock(Block *, Script *, Layout *, Layout *, u32 flags = 0);
void DrawGhostCommandBlock(Layout *layout);
void DrawGhostLoopBlock(Layout *layout, Layout *innerLayout = 0);

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

#define PushStruct(arena, type) (type *)PushSize(arena, sizeof(type))
#define PushArray(arena, type, count) (type *)PushSize(arena, sizeof(type) * count)
