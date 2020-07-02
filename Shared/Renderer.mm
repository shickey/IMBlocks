//
//  Renderer.m
//  IMBlocks Shared
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import <simd/simd.h>
#include <dlfcn.h>

#import "Renderer.h"
#import "Blocks.h"
#import "MetalView.h"

#include "imgui_impl_metal.h"
#include "imgui_impl_osx.h"

#import "BlocksInternal.h"

// Include header shared between C code here, which executes Metal API commands, and .metal files
#import "ShaderTypes.h"

static const u32 MAX_BUFFERS_IN_FLIGHT = 3;
static const u32 MAX_BLOCKS = 1024;

typedef void *DylibHandle;
typedef void(*InitBlocksSignature)(void *, u32);
typedef BlocksRenderInfo (*RunBlocksSignature)(void *, BlocksInput *);

struct WorldUniforms {
    float transform[16];
};

static NSDate *lastLibWriteTime = 0;
static DylibHandle libBlocks = 0;
static InitBlocksSignature initBlocks = 0;
static RunBlocksSignature runBlocks = 0;

static void *blocksMem = 0;

NSString *getLibPath() {
#if TARGET_OS_OSX
    NSString *appPath = [NSBundle.mainBundle bundlePath];
    NSString *directoryPath = [appPath stringByDeletingLastPathComponent];
    return [directoryPath stringByAppendingPathComponent:@"libBlocks.dylib"];
#else
    NSURL *frameworkUrl = [NSBundle.mainBundle URLForResource:@"libBlocksARM" withExtension:@"framework" subdirectory:@"Frameworks"];
    NSURL *dylibUrl = [frameworkUrl URLByAppendingPathComponent:@"libBlocksARM"];
    return [dylibUrl path];
#endif
}

NSDate *getLastWriteTime(NSString *filePath) {
    NSDictionary<NSFileAttributeKey, id> *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:filePath error:nil];
    return (NSDate *)attrs[NSFileModificationDate];
}

void loadLibBlocks() {
    NSLog(@"Loading libBlocks");
    NSString *libPath = getLibPath();
    const char *libPathRaw = [libPath fileSystemRepresentation];
    
    libBlocks = dlopen(libPathRaw, RTLD_LAZY|RTLD_LOCAL);
    initBlocks = (InitBlocksSignature)dlsym(libBlocks, "InitBlocks");
    runBlocks = (RunBlocksSignature)dlsym(libBlocks, "RunBlocks");
    lastLibWriteTime = getLastWriteTime(libPath);
}

void unloadLibBlocks() {
    NSLog(@"Unloading libBlocks");
    initBlocks = NULL;
    runBlocks = NULL;
    dlclose(libBlocks);
    libBlocks = NULL;
}

static f32 zoomLevel = 3.0;

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _vertBuffers[MAX_BUFFERS_IN_FLIGHT];
    id <MTLBuffer> _worldUniformsBuffers[MAX_BUFFERS_IN_FLIGHT];
    
    id <MTLTexture> blockTexture;
    
    id<MTLSamplerState> _sampler;
    
    id <MTLRenderPipelineState> _pipelineState;
    MTLVertexDescriptor *_mtlVertexDescriptor;

    uint8_t _bufferIndex;

    matrix_float4x4 _unprojectionMatrix;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self)
    {
        _device = view.device;
        _inFlightSemaphore = dispatch_semaphore_create(MAX_BUFFERS_IN_FLIGHT);
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplMetal_Init(_device);
        
        [self _loadMetalWithView:view];
    }

    return self;
}

- (void)_loadMetalWithView:(nonnull MTKView *)view;
{
    /// Load Metal state objects and initalize renderer dependent view properties

    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Set up vertex descriptor
    _mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];
    
        // Position
    _mtlVertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    _mtlVertexDescriptor.attributes[0].bufferIndex = 0;
    _mtlVertexDescriptor.attributes[0].offset = 0;
    
        // Tex UV
    _mtlVertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    _mtlVertexDescriptor.attributes[1].bufferIndex = 0;
    _mtlVertexDescriptor.attributes[1].offset = 2 * sizeof(f32);
    
        // Color
    _mtlVertexDescriptor.attributes[2].format = MTLVertexFormatFloat4;
    _mtlVertexDescriptor.attributes[2].bufferIndex = 0;
    _mtlVertexDescriptor.attributes[2].offset = 4 * sizeof(f32);
    
        // Stride
    _mtlVertexDescriptor.layouts[0].stride = 8 * sizeof(f32);
    _mtlVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    // Sampler
    MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToZero;
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToZero;
    _sampler = [_device newSamplerStateWithDescriptor:samplerDescriptor];
    
    // Set up shaders
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];
    id <MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"TexturedVertex"];
    id <MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"SdfFragment"];

    // Create rendering pipeline
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    NSError *error = NULL;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }

    // Allocate buffers for GPU data
    for(u32 i = 0; i < MAX_BUFFERS_IN_FLIGHT; ++i)
    {
        static const u32 BLOCK_BYTE_SIZE = 12 * 2 * sizeof(f32);
        _vertBuffers[i] = [_device newBufferWithLength:(BLOCK_BYTE_SIZE * MAX_BLOCKS) options:MTLResourceStorageModeShared];
        _vertBuffers[i].label = @"Vertex Buffer";
        
        _worldUniformsBuffers[i] = [_device newBufferWithLength:(sizeof(WorldUniforms) * 3) options:MTLResourceStorageModeShared];
        _worldUniformsBuffers[i].label = @"World Uniforms Buffer";
    }
    
    // Load block texture
    MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm 
                                                                                             width:512 
                                                                                            height:512 
                                                                                         mipmapped:false];
    blockTexture = [_device newTextureWithDescriptor:texDescriptor];
    
    NSData *texData = [NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"block-textures" withExtension:@"dat"]];
    [blockTexture replaceRegion:MTLRegionMake2D(0, 0, 512, 512) mipmapLevel:0 withBytes:texData.bytes bytesPerRow:512];
    
    // Init Blocks Memory
    loadLibBlocks();
    u32 memSize = Megabytes(128);
    blocksMem = malloc(memSize);
    initBlocks(blocksMem, memSize);

    _commandQueue = [_device newCommandQueue];
}

- (CGPoint)_unprojectPoint:(CGPoint)point inView:(MetalView *)view {
    CGFloat projectedX = (2.0 * (point.x / view.bounds.size.width)) - 1.0;
    CGFloat projectedY = (2.0 * (point.y / view.bounds.size.height)) - 1.0;
#if TARGET_OS_IPHONE
    projectedY *= -1.0; // Flip Y on iOS
#endif
    
    simd_float4 pVec = simd_make_float4(projectedX, projectedY, 0.0, 1.0);
    simd_float4 P = matrix_multiply(_unprojectionMatrix, pVec);
    return CGPointMake(P.x, P.y);
}

- (void)drawImGuiIn:(MetalView *)view
               with:(MTLRenderPassDescriptor *)renderPassDescriptor 
      commandBuffer:(id<MTLCommandBuffer>)commandBuffer
      renderEncoder:(id<MTLRenderCommandEncoder>)renderEncoder
{
    
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplOSX_NewFrame(view);
    ImGui::NewFrame();
    
//    static bool show_demo_window = true;
//    ImGui::ShowDemoWindow(&show_demo_window);
    
    {
        ImGui::Begin("Blocks Debug");                          // Create a window called "Hello, world!" and append into it.
        
        ImGui::SliderFloat("zoom", &zoomLevel, 0.5, 16.0);
        
        BlocksContext *ctx = (BlocksContext *)blocksMem;
        for (u32 i = 0; i < ctx->scriptCount; ++i) {
            Script *script = &ctx->scripts[i];
            char label[50];
            sprintf(label, "script %i", i);
            ImGui::InputFloat2(label, (float *)&(script->P));
        }

//        ImGui::ProgressBar((f32)ctx->blocks.used / (f32)ctx->blocks.size, ImVec2(0.0f, 0.0f), "Blocks Arena Usage");
//        ImGui::ProgressBar((f32)ctx->verts.used / (f32)ctx->verts.size, ImVec2(0.0f, 0.0f), "Vertex Arena Usage");
        ImGui::ProgressBar((f32)ctx->scriptCount / 1024.f, ImVec2(0.0f, 0.0f), "Script usage");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
}

- (void)drawInMTKView:(nonnull MetalView *)view
{
    // Load/Reload dylib if necessary
    if (!libBlocks || !lastLibWriteTime || [lastLibWriteTime compare:getLastWriteTime(getLibPath())] == NSOrderedAscending) {
        if (libBlocks) {
            unloadLibBlocks();
        }
        loadLibBlocks();
        if (!libBlocks) {
            NSLog(@"WARNING: Missed dylib reload");
            // The dylib may still be being written, so we spin until the next frame when (hopefully) it will be ready
            return;
        }
    }

    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    _bufferIndex = (_bufferIndex + 1) % MAX_BUFFERS_IN_FLIGHT;

    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_sema);
     }];
    
    Input input = view->_input;
    
    BlocksInput blocksInput;
    blocksInput.mouseP = {(f32)input.mouseX, (f32)input.mouseY};
    blocksInput.mouseDown = input.mouseDown;
    blocksInput.wheelDelta = {input.wheelDx, input.wheelDy};
    blocksInput.commandDown = input.commandDown;
    f32 dpi = (f32)view.window.backingScaleFactor;
    blocksInput.screenSize = {(f32)view.bounds.size.width / dpi, (f32)view.bounds.size.height / dpi};
    
    // Reset scroll deltas for next frame
    view->_input.wheelDx = 0;
    view->_input.wheelDy = 0;
    
    id <MTLBuffer> vertBuffer = _vertBuffers[_bufferIndex];
    
    BlocksRenderInfo renderInfo = runBlocks(blocksMem, &blocksInput);
    memcpy(vertBuffer.contents, renderInfo.vertexData, renderInfo.vertexDataSize);
    
    // World transform
    id <MTLBuffer> worldUniformsBuffer = _worldUniformsBuffers[_bufferIndex];
    WorldUniforms *worldUniforms = (WorldUniforms *)[worldUniformsBuffer contents];
    
    for (u32 i = 0; i < renderInfo.drawCallCount; ++i) {
        BlocksDrawCall *drawCall = &renderInfo.drawCalls[i];
        memcpy(worldUniforms + i, &drawCall->transform, sizeof(WorldUniforms));
    }
    
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    if(renderPassDescriptor != nil)
    {
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake((f64)0x33 / 255.0, (f64)0x47 / 255.0, (f64)0x71 / 255.0, 1.0);
        
        id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        // Render data from libBlocks
        [renderEncoder setVertexBuffer:vertBuffer offset:0 atIndex:0];
        [renderEncoder setVertexBuffer:worldUniformsBuffer offset:0 atIndex:1];
        
        [renderEncoder setFragmentTexture:blockTexture atIndex:0];
        [renderEncoder setFragmentSamplerState:_sampler atIndex:0];
        
        for (u32 i = 0; i < renderInfo.drawCallCount; ++i) {
            BlocksDrawCall *drawCall = &renderInfo.drawCalls[i];
            
            [renderEncoder setVertexBufferOffset:(i * sizeof(WorldUniforms)) atIndex:1];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle 
                              vertexStart:drawCall->vertexOffset 
                              vertexCount:drawCall->vertexCount];
        }
        
        [self drawImGuiIn:view 
                     with:renderPassDescriptor
            commandBuffer:commandBuffer
            renderEncoder:renderEncoder];

        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{   

}

#pragma mark Matrix Math Utilities

matrix_float4x4 translationMatrix(float tx, float ty, float tz)
{
    return (matrix_float4x4) {{
        { 1,   0,  0,  0 },
        { 0,   1,  0,  0 },
        { 0,   0,  1,  0 },
        { tx, ty, tz,  1 }
    }};
}

matrix_float4x4 rotationMatrix(float radians, vector_float3 axis)
{
    axis = vector_normalize(axis);
    float ct = cosf(radians);
    float st = sinf(radians);
    float ci = 1 - ct;
    float x = axis.x, y = axis.y, z = axis.z;

    return (matrix_float4x4) {{
        { ct + x * x * ci,     y * x * ci + z * st, z * x * ci - y * st, 0},
        { x * y * ci - z * st,     ct + y * y * ci, z * y * ci + x * st, 0},
        { x * z * ci + y * st, y * z * ci - x * st,     ct + z * z * ci, 0},
        {                   0,                   0,                   0, 1}
    }};
}

matrix_float4x4 perspectiveProjectionRightHand(float fovyRadians, float aspect, float nearZ, float farZ)
{
    float ys = 1 / tanf(fovyRadians * 0.5);
    float xs = ys / aspect;
    float zs = farZ / (nearZ - farZ);

    return (matrix_float4x4) {{
        { xs,   0,          0,  0 },
        {  0,  ys,          0,  0 },
        {  0,   0,         zs, -1 },
        {  0,   0, nearZ * zs,  0 }
    }};
}

matrix_float4x4 orthographicProjection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return (matrix_float4x4) {{
        { 2.0f / (right - left),                     0,                    0, -(right + left) / (right - left) },
        {                     0, 2.0f / (top - bottom),                    0, -(top + bottom) / (top - bottom) },
        {                     0,                     0, -2.0f / (far - near),     -(far + near) / (far - near) },
        {                     0,                     0,                    0,                             1.0f }
    }};
}

matrix_float4x4 orthographicUnprojection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
    return (matrix_float4x4) {{
        { (right - left) / 2.0f,                     0,                    0, (right + left) / 2.0f },
        {                     0, (top - bottom) / 2.0f,                    0, (top + bottom) / 2.0f },
        {                     0,                     0, (far - near) / -2.0f,   (far + near) / 2.0f },
        {                     0,                     0,                    0,                  1.0f }
    }};
}

@end
