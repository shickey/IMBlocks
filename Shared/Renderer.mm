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

// Include header shared between C code here, which executes Metal API commands, and .metal files
#import "ShaderTypes.h"

static const u32 MAX_BUFFERS_IN_FLIGHT = 3;
static const u32 MAX_BLOCKS = 1024;

typedef void *DylibHandle;
typedef void(*InitBlocksSignature)(void *, u32);
typedef BlocksRenderInfo (*RunBlocksSignature)(void *, BlocksInput *);

struct WorldUniforms {
    matrix_float4x4 transform;
};

struct DebugRect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

static u32 debugRectCount = 0;

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

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;

    id <MTLBuffer> _vertBuffers[MAX_BUFFERS_IN_FLIGHT];
    id <MTLBuffer> _blockUniformsBuffers[MAX_BUFFERS_IN_FLIGHT];
    id <MTLBuffer> _worldUniformsBuffers[MAX_BUFFERS_IN_FLIGHT];
    id <MTLBuffer> _debugVertBuffers[MAX_BUFFERS_IN_FLIGHT];
    
    id <MTLTexture> blockTexture;
    
    id<MTLSamplerState> _sampler;
    
    id <MTLRenderPipelineState> _pipelineState;
    id <MTLRenderPipelineState> _debugPipelineState;
    MTLVertexDescriptor *_mtlVertexDescriptor;

    uint8_t _bufferIndex;

    matrix_float4x4 _projectionMatrix;
    matrix_float4x4 _unprojectionMatrix;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view;
{
    self = [super init];
    if(self)
    {
        _device = view.device;
        _inFlightSemaphore = dispatch_semaphore_create(MAX_BUFFERS_IN_FLIGHT);
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
    
        // Tex UV
    _mtlVertexDescriptor.attributes[2].format = MTLVertexFormatFloat3;
    _mtlVertexDescriptor.attributes[2].bufferIndex = 0;
    _mtlVertexDescriptor.attributes[2].offset = 4 * sizeof(f32);
    
        // Stride
    _mtlVertexDescriptor.layouts[0].stride = 7 * sizeof(f32);
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
    id <MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"simple_vertex"];
    id <MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"simple_fragment"];
    id <MTLFunction> debugVertexFunction = [defaultLibrary newFunctionWithName:@"debug_vertex"];

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
    
    // Create debug pipeline
    MTLRenderPipelineDescriptor *debugPipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    debugPipelineStateDescriptor.vertexFunction = debugVertexFunction;
    debugPipelineStateDescriptor.fragmentFunction = fragmentFunction;
    debugPipelineStateDescriptor.vertexDescriptor = _mtlVertexDescriptor;
    debugPipelineStateDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    
    error = NULL;
    _debugPipelineState = [_device newRenderPipelineStateWithDescriptor:debugPipelineStateDescriptor error:&error];
    if (!_debugPipelineState)
    {
        NSLog(@"Failed to created debug pipeline state, error %@", error);
    }

    // Allocate buffers for GPU data
    for(u32 i = 0; i < MAX_BUFFERS_IN_FLIGHT; ++i)
    {
        static const u32 BLOCK_BYTE_SIZE = 12 * 2 * sizeof(f32);
        _vertBuffers[i] = [_device newBufferWithLength:(BLOCK_BYTE_SIZE * MAX_BLOCKS) options:MTLResourceStorageModeShared];
        _vertBuffers[i].label = @"Vertex Buffer";
        
//        _blockUniformsBuffers[i] = [_device newBufferWithLength:(sizeof(BlockUniforms) * MAX_BLOCKS) options:MTLResourceStorageModeShared];
//        _blockUniformsBuffers[i].label = @"Block Uniforms Buffer";
        
        _worldUniformsBuffers[i] = [_device newBufferWithLength:(sizeof(WorldUniforms)) options:MTLResourceStorageModeShared];
        _worldUniformsBuffers[i].label = @"World Uniforms Buffer";
        
        _debugVertBuffers[i] = [_device newBufferWithLength:(16 * 256) options:MTLResourceStorageModeShared];
        _debugVertBuffers[i].label = @"Debug Vertex Buffer";
    }
    
    // Load block texture
    MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm 
                                                                                             width:512 
                                                                                            height:512 
                                                                                         mipmapped:false];
    blockTexture = [_device newTextureWithDescriptor:texDescriptor];
    
    NSData *texData = [NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"block-textures" withExtension:@"dat"]];
//    u8 *texBytes = (u8 *)texData.bytes;
//    f32 *texFloats = (f32 *)malloc(512 * 512 * sizeof(f32));
//    for(u32 i = 0; i < 512 * 512; ++i) {
//        texFloats[i] = texBytes[i] / 255.0f;
//    }
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

//- (void)renderDebugRectsInBuffer:(id<MTLBuffer>)buffer {
//    for (u32 i = 0; i < 4; ++i) {
//        BlockData data = blockData[i];
//        DebugRect rect = {data.x + 16, data.y, 12, 16};
//        [self pushDebugRectVerts:rect inBuffer:buffer];
//    }
//}

- (void)pushDebugRectVerts:(DebugRect)rect inBuffer:(id<MTLBuffer>)buffer {
    f32 *vertStorage = ((f32 *)buffer.contents) + ((debugRectCount++) * 16);
    f32 verts[] = {
        // Bottom
        rect.x, rect.y,
        rect.x + rect.w, rect.y,
        
        // Right
        rect.x + rect.w, rect.y,
        rect.x + rect.w, rect.y + rect.h,
        
        // Top
        rect.x + rect.w, rect.y + rect.h,
        rect.x, rect.y + rect.h,
        
        // Left
        rect.x, rect.y + rect.h,
        rect.x, rect.y,
    };
    memcpy(vertStorage, verts, sizeof(verts));
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
    
    // Clear debug rects
    debugRectCount = 0;

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
    CGPoint P = [self _unprojectPoint:CGPointMake(input.mouseX, input.mouseY) inView:view];
    
    BlocksInput blocksInput;
    blocksInput.mouseP = {(f32)P.x, (f32)P.y};
    blocksInput.mouseDown = input.mouseDown;
    
    id <MTLBuffer> vertBuffer = _vertBuffers[_bufferIndex];
    id <MTLBuffer> blockUniformsBuffer = _blockUniformsBuffers[_bufferIndex];
    
    BlocksRenderInfo renderInfo = runBlocks(blocksMem, &blocksInput);
    memcpy(vertBuffer.contents, renderInfo.verts, renderInfo.vertsSize);
    
//    id <MTLBuffer> debugVertBuffer = _debugVertBuffers[_bufferIndex];
//    [self renderDebugRectsInBuffer:debugVertBuffer];
    
    // World transform
    id <MTLBuffer> worldUniformsBuffer = _worldUniformsBuffers[_bufferIndex];
    WorldUniforms *worldUniforms = (WorldUniforms *)[worldUniformsBuffer contents];
    worldUniforms->transform = _projectionMatrix;

    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    if(renderPassDescriptor != nil)
    {
        id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

        // Blocks
        [renderEncoder setRenderPipelineState:_pipelineState];

        [renderEncoder setVertexBuffer:vertBuffer offset:0 atIndex:0];
        [renderEncoder setVertexBuffer:blockUniformsBuffer offset:0 atIndex:1];
        [renderEncoder setVertexBuffer:worldUniformsBuffer offset:0 atIndex:2];
        
        [renderEncoder setFragmentTexture:blockTexture atIndex:0];
        [renderEncoder setFragmentSamplerState:_sampler atIndex:0];
        
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:renderInfo.vertsCount];
        
        // Debug Drawing
//        [renderEncoder setRenderPipelineState:_debugPipelineState];
//
//        [renderEncoder setVertexBuffer:debugVertBuffer offset:0 atIndex:0];
//        [renderEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:8 * debugRectCount];

        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{   
    static f32 zoomLevel = 8.0;
    
//    f32 aspect = size.width / (float)size.height;
    
    CGFloat halfWidth = (size.width / 2.0) / zoomLevel;
    CGFloat halfHeight = (size.height / 2.0) / zoomLevel;
    
    _projectionMatrix = orthographicProjection(-halfWidth, halfWidth, halfHeight, -halfHeight, 1.0, -1.0);
    _unprojectionMatrix = orthographicUnprojection(-halfWidth, halfWidth, halfHeight, -halfHeight, 1.0, -1.0);
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
