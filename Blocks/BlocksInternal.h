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
struct RenderBasis;

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

struct RenderBasis {
    v2 at;
    v2 bounds;
};

enum RenderEntryType {
    RenderEntryType_Command,
    RenderEntryType_Loop
};

struct RenderEntry {
    RenderingIndex idx;
    RenderEntryType type;
    v2 P;
    v3 color;
    union {
        struct {
            u32 hStretch;
            u32 vStretch;
        };
    };
};

struct Interactable {
    RenderingIndex renderingIdx; // 0 indicates none
    Script *script;
    v2 blockP;
    
    v2 mouseOffset;
};

struct Block {
    Block *prev;
    Block *next;
    BlockType type;
    
    // Loops
    Block *inner;
};

struct Script {
    v2 P;
    Block *topBlock;
};

struct BlocksRect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

struct BlocksContext {
    BlocksInput input;
    
    Arena verts;
    Arena blocks;
    
    RenderEntry renderEntries[4096];
    RenderingIndex nextRenderingIdx;
    
    Script scripts[1024];
    u32 scriptCount;
    
    Interactable hot;
    Interactable interacting;
    Interactable nextHot;
};

void BeginBlocks(BlocksInput input);
BlocksRenderInfo EndBlocks(void);
void DrawBlock(Block *, Script *, RenderBasis *);
void DrawCommandBlock(Block *, Script *, RenderBasis *);
void DrawLoopBlock(Block *, Script *, RenderBasis *);


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
