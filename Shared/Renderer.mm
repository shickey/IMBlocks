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
static char **shaderSource = 0;

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
    shaderSource = (char **)dlsym(libBlocks, "BlocksShaders_Metal");
    lastLibWriteTime = getLastWriteTime(libPath);
}

void unloadLibBlocks() {
    NSLog(@"Unloading libBlocks");
    initBlocks = NULL;
    runBlocks = NULL;
    shaderSource = NULL;
    dlclose(libBlocks);
    libBlocks = NULL;
}

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _vertBuffers[MAX_BUFFERS_IN_FLIGHT];
    id <MTLBuffer> _worldUniformsBuffers[MAX_BUFFERS_IN_FLIGHT];
    
    id <MTLTexture> blockSdfTexture;
    id <MTLTexture> fontSdfTexture;
    id <MTLTexture> blockMipTexture;
    
    id<MTLSamplerState> _sampler;
    
    id <MTLRenderPipelineState> _sdfPipelineState;
    id <MTLRenderPipelineState> _mipPipelineState;
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
    
        // Outline Color
    _mtlVertexDescriptor.attributes[3].format = MTLVertexFormatFloat4;
    _mtlVertexDescriptor.attributes[3].bufferIndex = 0;
    _mtlVertexDescriptor.attributes[3].offset = 8 * sizeof(f32);
    
        // Stride
    _mtlVertexDescriptor.layouts[0].stride = 12 * sizeof(f32);
    _mtlVertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    // Sampler
    MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToZero;
    samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToZero;
    _sampler = [_device newSamplerStateWithDescriptor:samplerDescriptor];
    
    // Load the dylib
    loadLibBlocks();
    
    // Setup rendering pipelines
    [self buildRenderPipelines];

    // Allocate buffers for GPU data
    for(u32 i = 0; i < MAX_BUFFERS_IN_FLIGHT; ++i)
    {
        static const u32 BLOCK_BYTE_SIZE = 12 * 2 * sizeof(f32);
        _vertBuffers[i] = [_device newBufferWithLength:(BLOCK_BYTE_SIZE * MAX_BLOCKS) options:MTLResourceStorageModeShared];
        _vertBuffers[i].label = @"Vertex Buffer";
        
        _worldUniformsBuffers[i] = [_device newBufferWithLength:(sizeof(WorldUniforms) * 5) options:MTLResourceStorageModeShared];
        _worldUniformsBuffers[i].label = @"World Uniforms Buffer";
    }
    
    // Load SDF block texture
    MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm 
                                                                                             width:512 
                                                                                            height:512 
                                                                                         mipmapped:false];
    blockSdfTexture = [_device newTextureWithDescriptor:texDescriptor];
    
    NSData *texData = [NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"blocks-atlas" withExtension:@"dat"]];
    [blockSdfTexture replaceRegion:MTLRegionMake2D(0, 0, 512, 512) mipmapLevel:0 withBytes:texData.bytes bytesPerRow:512];
    
    // Load font texture
    fontSdfTexture = [_device newTextureWithDescriptor:texDescriptor];
    
    NSData *fontData = [NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"font-atlas" withExtension:@"dat"]];
    [fontSdfTexture replaceRegion:MTLRegionMake2D(0, 0, 512, 512) mipmapLevel:0 withBytes:fontData.bytes bytesPerRow:512];
    
    // Load mipmapped textures
    MTLTextureDescriptor *mipTexDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm 
                                                                                                width:512 
                                                                                               height:512 
                                                                                            mipmapped:true];
    blockMipTexture = [_device newTextureWithDescriptor:mipTexDescriptor];
    u32 totalMipLevels = ceil(log2(512));
    for (u32 i = 512; i > 0; i /= 2) {
        u32 mipLevel = totalMipLevels - (ceil(log2(i)));
        NSString *filename = [NSString stringWithFormat:@"blocks-mip-%i", i];
        NSData *texData = [NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:filename withExtension:@"dat"]];
        [blockMipTexture replaceRegion:MTLRegionMake2D(0, 0, i, i) mipmapLevel:mipLevel withBytes:texData.bytes bytesPerRow:i];
    }
    
    // Init Blocks Memory
    u32 memSize = Megabytes(128);
    blocksMem = malloc(memSize);
    initBlocks(blocksMem, memSize);

    _commandQueue = [_device newCommandQueue];
}

- (void)buildRenderPipelines {
    // Set up shaders
    NSString *nsShaderSource = [NSString stringWithUTF8String:*shaderSource];
    
    NSError *shaderError = nil;
    id<MTLLibrary> library = [_device newLibraryWithSource:nsShaderSource options:nil error:&shaderError];
    if (shaderError) {
        NSLog(@"%@", shaderError.localizedDescription);
    }
    id <MTLFunction> vertexFunction = [library newFunctionWithName:@"TexturedVertex"];
    id <MTLFunction> sdfFragmentFunction = [library newFunctionWithName:@"SdfFragment"];
    id <MTLFunction> mipFragmentFunction = [library newFunctionWithName:@"MipmapFragment"];

    // Create rendering pipelines
    MTLRenderPipelineDescriptor *sdfPipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    sdfPipelineStateDescriptor.vertexFunction = vertexFunction;
    sdfPipelineStateDescriptor.fragmentFunction = sdfFragmentFunction;
    sdfPipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    sdfPipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    sdfPipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
    sdfPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    sdfPipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    sdfPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    sdfPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    sdfPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    sdfPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    NSError *error = NULL;
    _sdfPipelineState = [_device newRenderPipelineStateWithDescriptor:sdfPipelineStateDescriptor error:&error];
    if (!_sdfPipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }
    
    MTLRenderPipelineDescriptor *mipPipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    mipPipelineStateDescriptor.vertexFunction = vertexFunction;
    mipPipelineStateDescriptor.fragmentFunction = mipFragmentFunction;
    mipPipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    mipPipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    mipPipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
    mipPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    mipPipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    mipPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    mipPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    mipPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    mipPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    error = NULL;
    _mipPipelineState = [_device newRenderPipelineStateWithDescriptor:mipPipelineStateDescriptor error:&error];
    if (!_mipPipelineState)
    {
        NSLog(@"Failed to created pipeline state, error %@", error);
    }
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

- (void)beginImGuiWithView:(MetalView *)view renderPassDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor {
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplOSX_NewFrame(view);
    ImGui::NewFrame();
}

- (void)endImGuiWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer renderEncoder:(id<MTLRenderCommandEncoder>)renderEncoder {
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
}

//- (void)drawImGuiIn:(MetalView *)view
//               with:(MTLRenderPassDescriptor *)renderPassDescriptor 
//      commandBuffer:(id<MTLCommandBuffer>)commandBuffer
//      renderEncoder:(id<MTLRenderCommandEncoder>)renderEncoder
//{
//    
//    
//    
////    static bool show_demo_window = true;
////    ImGui::ShowDemoWindow(&show_demo_window);
//    
////    {
////        ImGui::Begin("Blocks Debug");                          // Create a window called "Hello, world!" and append into it.
////        
////        ImGui::SliderFloat("zoom", &zoomLevel, 0.5, 16.0);
////        
////        BlocksContext *ctx = (BlocksContext *)blocksMem;
////        for (u32 i = 0; i < ctx->scriptCount; ++i) {
////            Script *script = &ctx->scripts[i];
////            char label[50];
////            sprintf(label, "script %i", i);
////            ImGui::InputFloat2(label, (float *)&(script->P));
////        }
////
//////        ImGui::ProgressBar((f32)ctx->blocks.used / (f32)ctx->blocks.size, ImVec2(0.0f, 0.0f), "Blocks Arena Usage");
//////        ImGui::ProgressBar((f32)ctx->verts.used / (f32)ctx->verts.size, ImVec2(0.0f, 0.0f), "Vertex Arena Usage");
////        ImGui::ProgressBar((f32)ctx->scriptCount / 1024.f, ImVec2(0.0f, 0.0f), "Script usage");
////
////        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
////        ImGui::End();
////    }
//}

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
        [self buildRenderPipelines]; // Rebuild shaders and such
    }

    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    _bufferIndex = (_bufferIndex + 1) % MAX_BUFFERS_IN_FLIGHT;

    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"BlocksCommandBuffer";

    __block dispatch_semaphore_t block_sema = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
     {
         dispatch_semaphore_signal(block_sema);
     }];
    
    Input input = view->_input;
    
    f32 dpi = (f32)view.window.backingScaleFactor;
    
    BlocksInput blocksInput;
    blocksInput.mouseP = {(f32)input.mouseX / dpi, (f32)input.mouseY / dpi};
    blocksInput.mouseDown = input.mouseDown;
    blocksInput.wheelDelta = {input.wheelDx, input.wheelDy};
    blocksInput.commandDown = input.commandDown;
    blocksInput.screenSize = {(f32)view.bounds.size.width / dpi, (f32)view.bounds.size.height / dpi};
    
    // Reset scroll deltas for next frame
    view->_input.wheelDx = 0;
    view->_input.wheelDy = 0;
    
    id <MTLBuffer> vertBuffer = _vertBuffers[_bufferIndex];
    id <MTLBuffer> worldUniformsBuffer = _worldUniformsBuffers[_bufferIndex];
    
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    if(renderPassDescriptor != nil)
    {
        [self beginImGuiWithView:view renderPassDescriptor:renderPassDescriptor]; 
        
        BlocksRenderInfo renderInfo = runBlocks(blocksMem, &blocksInput);
        memcpy(vertBuffer.contents, renderInfo.vertexData, renderInfo.vertexDataSize);
        
        WorldUniforms *worldUniforms = (WorldUniforms *)[worldUniformsBuffer contents];
        for (u32 i = 0; i < renderInfo.drawCallCount; ++i) {
            BlocksDrawCall *drawCall = &renderInfo.drawCalls[i];
            memcpy(worldUniforms + i, &drawCall->transform, sizeof(WorldUniforms));
        }
        
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake((f64)0x33 / 255.0, (f64)0x47 / 255.0, (f64)0x71 / 255.0, 1.0);
        
        id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"BlocksRenderEncoder";
        
        [renderEncoder setRenderPipelineState:_sdfPipelineState];
        
        // Render data from libBlocks
        [renderEncoder setVertexBuffer:vertBuffer offset:0 atIndex:0];
        [renderEncoder setVertexBuffer:worldUniformsBuffer offset:0 atIndex:1];
        
        [renderEncoder setFragmentTexture:blockSdfTexture atIndex:0];
        [renderEncoder setFragmentSamplerState:_sampler atIndex:0];
        
        for (u32 i = 0; i < renderInfo.drawCallCount; ++i) {
            BlocksDrawCall *drawCall = &renderInfo.drawCalls[i];
            
            if (i == renderInfo.drawCallCount - 1) {
                [renderEncoder setFragmentTexture:fontSdfTexture atIndex:0];
            }
            
            [renderEncoder setVertexBufferOffset:(i * sizeof(WorldUniforms)) atIndex:1];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle 
                              vertexStart:drawCall->vertexOffset 
                              vertexCount:drawCall->vertexCount];
        }
        
        [self endImGuiWithCommandBuffer:commandBuffer renderEncoder:renderEncoder];

        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    
}

@end
