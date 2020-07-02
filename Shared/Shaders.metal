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
    float4 color    [[ attribute(2) ]];
};

struct VertexOut {
    uint idx;
    float4 position [[ position ]];
    float2 texCoord;
    float4 color;
};

struct WorldUniforms {
    float4x4 transform;
};

vertex VertexOut simple_vertex(VertexIn in [[ stage_in ]],
                               const device WorldUniforms *worldUniforms [[ buffer(1) ]],
                               unsigned int vid [[ vertex_id ]]) {
    float4x4 projection = worldUniforms[0].transform;
    
    VertexOut out;
    out.texCoord = in.texCoord;
    out.position = projection * float4(in.position.x, in.position.y, 0, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 simple_fragment(VertexOut v [[ stage_in ]],
                                sampler samplr [[sampler(0)]],
                                texture2d<float, access::sample> blockTex [[ texture(0) ]]) {
    
    float edgeDistance = 0.5;
    float dist = blockTex.sample(samplr, v.texCoord).r;
    float dx = dfdx(dist);
    float dy = dfdy(dist);
    float edgeWidth = 1.0 * length(float2(dx, dy));
    float opacity = smoothstep(edgeDistance - edgeWidth, edgeDistance + edgeWidth, dist);
    
    return float4(v.color.rgb, opacity * v.color.a);
}

