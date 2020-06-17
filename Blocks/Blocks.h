//
//  blocks.hpp
//  gpu-blocks-test
//
//  Created by Sean Hickey on 6/10/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#ifndef blocks_h
#define blocks_h

#include <stdint.h>

#define internal static
#define global_var static
#define local_persist static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

typedef uint32_t b32;
typedef uint64_t b64;

struct V2 {
    f32 x;
    f32 y;
};

struct V3 {
    f32 x;
    f32 y;
    f32 z;
};

struct V4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct BlocksRect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

#define Kilobytes(num) (num * 1024LL)
#define Megabytes(num) (num * 1024LL * 1024LL)
#define Gigabytes(num) (num * 1024LL * 1024LL * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

#define Assert(expr) if(!(expr)) { *(volatile u32 *)0 = 0; }

typedef u32 BlockId;

struct BlockUniforms {
    u32 idx;
    f32 xOffset;
    f32 yOffset;
    b32 hot;
};

struct BlocksInput {
    union {
        V2 mouseP;
        V2 touchP;
        V2 P;
    };
    union {
        b32 mouseDown;
        b32 touching;
    };
};

enum BlockType {
    Command = 1,
};

struct BlocksRenderInfo {
    void *verts;
    u32 vertsSize;
    u32 vertsCount;
    void *uniforms;
    u32 uniformsSize;
};

void BeginBlocks(BlocksInput input);
BlocksRenderInfo EndBlocks(void);

void Block(BlockId id, BlockType type, f32 *x, f32 *y);

extern "C" {
void InitBlocks(void *mem, u32 memSize);
BlocksRenderInfo RunBlocks(void *mem, BlocksInput *input);
}

#endif /* blocks_h */
