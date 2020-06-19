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

struct VertexIn {
    float2 position [[ attribute(0) ]];  
    float2 texCoord [[ attribute(1) ]];
    float3 color    [[ attribute(2) ]];
};

struct VertexOut {
    uint idx;
    float4 position [[ position ]];
    float2 texCoord;
    float3 color;
};

//struct BlockUniforms {
//    uint idx;
//    float x;
//    float y;
//    bool hot;
//};

struct WorldUniforms {
    float4x4 transform;
};

vertex VertexOut simple_vertex(VertexIn in [[ stage_in ]],
//                               const device BlockUniforms *blockUniforms [[ buffer(1) ]],
                               const device WorldUniforms *worldUniforms [[ buffer(2) ]],
                               unsigned int vid [[ vertex_id ]]) {
    float4x4 projection = worldUniforms[0].transform;
//    BlockUniforms unis = blockUniforms[vid / 42];
    
    VertexOut out;
//    out.idx = unis.idx;
    out.texCoord = in.texCoord;
    out.position = projection * float4(in.position.x, in.position.y, 0, 1.0);
    out.color = in.color;
//    if (unis.hot) {
//        out.color = float3(1, 1, 0); // yellow
//    }
//    else {
//        uint colorIdx = unis.idx % 4;
//        if (colorIdx == 0) {
//            out.color = float3(1, 0, 0);
//        }
//        else if (colorIdx == 1) {
//            out.color = float3(0, 1, 0);
//        }
//        else if (colorIdx == 2) {
//            out.color = float3(0, 0, 1);
//        }
//        else {
//            out.color = float3(1, 0, 1);
//        }
//    }
    return out;
}

fragment float4 simple_fragment(VertexOut v [[ stage_in ]],
                                texture2d<float> blockTex [[ texture(0) ]]) {
    constexpr sampler s(coord::normalized, filter::linear);
    
//    float edgeDistance = 0.5;
//    float dist = (float)blockTex.sample(s, v.texCoord).r;
////    float dist = (float)blockTex.sample(s, v.texCoord).r / 255.0;
//    float edgeWidth = 0.75 * length(float2(dfdx(dist), dfdy(dist)));
//    float opacity = smoothstep(edgeDistance - edgeWidth, edgeDistance + edgeWidth, dist);
//    
//    return float4(v.color.rgb, opacity);
    
//    uint edgeDistance = 127;
    float dist = blockTex.sample(s, v.texCoord).r;
    if (dist < 0.5) {
        discard_fragment();
    }
    if (dist < 0.52) {
        return float4(v.color.r * 3.0 / 4.0, v.color.g * 3.0 / 4.0, v.color.b * 3.0 / 4.0, 1.0);
    }
    return float4(v.color.rgb, 1.0);
}


// Debug Rect drawing

vertex VertexOut debug_vertex(VertexIn in [[ stage_in ]],
                              const device WorldUniforms *worldUniforms [[ buffer(2) ]]) {
    float4x4 projection = worldUniforms[0].transform;
    
    VertexOut out;
    out.position = projection * float4(in.position, 0, 1.0);
    out.color = float3(1, 0, 0); // Red
    return out;
}

