/*********************************************************
*
* BlocksShaders.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

extern "C" const char *BlocksShaders_Metal = R"END(
#include <metal_stdlib>
#include <simd/simd.h>

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

vertex VertexOut TexturedVertex(VertexIn in [[ stage_in ]],
                                const device WorldUniforms *worldUniforms [[ buffer(1) ]],
                                unsigned int vid [[ vertex_id ]]) {
    float4x4 transform = worldUniforms[0].transform;
    
    VertexOut out;
    out.texCoord = in.texCoord;
    out.position = transform * float4(in.position.x, in.position.y, 0, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 SdfFragment(VertexOut v [[ stage_in ]],
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

fragment float4 MipmapFragment(VertexOut v [[ stage_in ]],
                               sampler samplr [[sampler(0)]],
                               texture2d<float, access::sample> blockTex [[ texture(0) ]]) {
    
    float opacity = blockTex.sample(samplr, v.texCoord).r;
    return float4(v.color.rgb, opacity * v.color.a);
}
)END";
