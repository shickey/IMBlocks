/*********************************************************
*
* BlocksTypes.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#include <stdint.h>
#include <string.h> // @TODO: We use this only for memcpy. Can we get rid of it eventually?

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

#define Kilobytes(num) (num * 1024LL)
#define Megabytes(num) (num * 1024LL * 1024LL)
#define Gigabytes(num) (num * 1024LL * 1024LL * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

#define Assert(expr) if(!(expr)) { *(volatile u32 *)0 = 0; }

union v2 {
    struct {
        f32 x;
        f32 y;
    };
    struct {
        f32 w;
        f32 h;
    };
};

union v3 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    struct {
        v2 xy;
        f32 _unusedZ;
    };
    struct {
        f32 _unusedX;
        v2 yz;
    };
    struct {
        v2 rg;
        f32 _unusedB;
    };
    struct {
        f32 _unusedR;
        v2 gb;
    };
};

union v4 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    struct {
        v3 xyz;
        f32 _unusedW;
    };
    struct {
        v3 rgb;
        f32 _unusedA;
    };
};
