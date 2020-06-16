//
//  Shaders.metal
//  IMBlocks Shared
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

// File for Metal kernel and shader functions

#include <metal_stdlib>
#include <simd/simd.h>

// Including header shared between this Metal shader code and Swift/C code executing Metal API commands
#import "ShaderTypes.h"

using namespace metal;

//typedef struct
//{
//    float3 position [[attribute(VertexAttributePosition)]];
//    float2 texCoord [[attribute(VertexAttributeTexcoord)]];
//} Vertex;
//
//typedef struct
//{
//    float4 position [[position]];
//    float2 texCoord;
//} ColorInOut;
//
//vertex ColorInOut vertexShader(Vertex in [[stage_in]],
//                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]])
//{
//    ColorInOut out;
//
//    float4 position = float4(in.position, 1.0);
//    out.position = uniforms.projectionMatrix * uniforms.modelViewMatrix * position;
//    out.texCoord = in.texCoord;
//
//    return out;
//}
//
//fragment float4 fragmentShader(ColorInOut in [[stage_in]],
//                               constant Uniforms & uniforms [[ buffer(BufferIndexUniforms) ]],
//                               texture2d<half> colorMap     [[ texture(TextureIndexColor) ]])
//{
//    constexpr sampler colorSampler(mip_filter::linear,
//                                   mag_filter::linear,
//                                   min_filter::linear);
//
//    half4 colorSample   = colorMap.sample(colorSampler, in.texCoord.xy);
//
//    return float4(colorSample);
//}

struct VertexIn {
    float2 position [[ attribute(0) ]];  
};

struct VertexOut {
    uint idx;
    float4 position [[ position ]];
    float3 color;
};

//struct FragmentOut {
//    float4 image [[ color(0) ]];
//    uint picking [[ color(1) ]];
//};

struct BlockUniforms {
    uint idx;
    float x;
    float y;
    bool hot;
};

struct WorldUniforms {
    float4x4 transform;
};

vertex VertexOut simple_vertex(VertexIn in [[ stage_in ]],
                               device BlockUniforms *blockUniforms [[ buffer(1) ]],
                               device WorldUniforms *worldUniforms [[ buffer(2) ]],
                               unsigned int vid [[ vertex_id ]]) {
    float4x4 projection = worldUniforms[0].transform;
    BlockUniforms unis = blockUniforms[vid / 36];
    
    VertexOut out;
    out.idx = unis.idx;
    out.position = projection * float4(in.position.r + unis.x, in.position.g + unis.y, 0, 1.0);
    if (unis.hot) {
        out.color = float3(1, 1, 0); // yellow
    }
    else {
        out.color = float3(0, 1, 1); // cyan
    }
    return out;
}

fragment float4 simple_fragment(VertexOut v [[ stage_in ]]) {
    return float4(v.color.rgb, 1.0);
}


// Debug Rect drawing

vertex VertexOut debug_vertex(VertexIn in [[ stage_in ]],
                              device WorldUniforms *worldUniforms [[ buffer(2) ]]) {
    float4x4 projection = worldUniforms[0].transform;
    
    VertexOut out;
    out.position = projection * float4(in.position, 0, 1.0);
    out.color = float3(1, 0, 0); // Red
    return out;
}

