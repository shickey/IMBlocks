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

typedef u32 BlockId;
typedef u32 ScriptId;

enum BlockType {
    Command = 1,
    Loop
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

struct Interactable {
    ScriptId id;
    f32 *x;
    f32 *y;
    
    v2 mouseOffset;
};

struct Block {
    Script *script;
    BlockType type;
    Block *next;
    
    // Loop
    Block *inner;
};

struct Script {
    ScriptId id;
    f32 x;
    f32 y;
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
    
    Script scripts[1024];
    u32 scriptCount;
    
    Interactable hot;
    Interactable interacting;
    Interactable nextHot;
};

void BeginBlocks(BlocksInput input);
BlocksRenderInfo EndBlocks(void);
void DrawBlock(Block *, RenderBasis *);
void DrawCommandBlock(Block *, RenderBasis *);
void DrawLoopBlock(Block *, RenderBasis *);


void *pushSize(Arena *arena, u32 size) {
  // Make sure we have enough space left in the arena
  Assert(arena->used + size <= arena->size);
  
  void *result = arena->data + arena->used;
  arena->used += size;
  
  return result;
}

void pushData_(Arena *arena, void *data, u32 size) {
    void *location = pushSize(arena, size);
    memcpy(location, data, size);
}

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))
#define pushArray(arena, type, count) (type *)pushSize(arena, sizeof(type) * count)
