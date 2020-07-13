//
//  main.m
//  GenTextures
//
//  Created by Sean Hickey on 6/17/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <stdint.h>
#import <CoreText/CoreText.h>

#define GENERATE_SDF 0
#define GENERATE_MIPMAPS 0
#define GENERATE_FONT 1
#define OUTPUT_SDF_IMAGE 1

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

#define Assert(expr) if(!(expr)) { *(volatile u32 *)0 = 0; }



typedef struct Pt {
    s32 x;
    s32 y;
} Pt;



f32 clamp(f32 val, f32 min, f32 max) {
    return (val < min) ? min : (val > max ? max : val);
}
f32 *deadReckoning(u8 *pixels, u32 width, u32 height);
void generateSdfTexture(void);
void generateMipmaps(void);
void generateFont(void);

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        #if GENERATE_SDF
        generateSdfTexture();  
        #endif
        #if GENERATE_MIPMAPS
        generateMipmaps();
        #endif
        #if GENERATE_FONT
        generateStbFont();
        #endif
    }
    return 0;
}

//typedef struct KerningTableHeader {
//    u32 version;
//    u32 nTables;
//} KerningTableHeader;
//
//typedef struct KerningSubTableHeader {
//    u32 length;
//    u16 coverage;
//    u16 tupleIndex;
//} KerningSubTableHeader;
//
//typedef struct KerningFormat0Header {
//    u16 nPairs;
//    u16 searchPairs;
//    u16 entrySelector;
//    u16 rangeShift;
//} KerningFormat0Header;
//
//typedef struct KerningPair {
//    u16 l;
//    u16 r;
//    s16 val;
//} KerningPair;
//
//typedef struct SimpleFontHeader {
//    KerningTableHeader tableHeader;
//    KerningSubTableHeader subTableHeader;
//    KerningFormat0Header format0Header;
//} SimpleFontHeader;

void generateFont() {
    
}

//void generateFont() {
//    CTFontRef font = CTFontCreateWithName((__bridge CFStringRef)@"Helvetica Neue Bold", 256.0, nil);
//  
//    char *chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789()[]{};:'\"<>?,./\\!@#$%^&*-_+=~";
//    u32 nChars = (u32)strlen(chars);
//    UniChar *unichars = malloc(nChars * sizeof(UniChar));
//    for (u32 i = 0; i < nChars; ++i) {
//        unichars[i] = (UniChar)chars[i];
//    }
//    
//    // Extract glyphs
//    CGGlyph *glyphs = malloc(nChars * sizeof(CGGlyph));
//    b32 success = CTFontGetGlyphsForCharacters(font, unichars, glyphs, nChars);
//    Assert(success);
//    
//    // Extract bounding boxes
//    CGRect *boundingBoxes = malloc(nChars * sizeof(CGRect));
//    CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, glyphs, boundingBoxes, nChars);
//    
//    // Extract advances
//    CGSize *advances = malloc(nChars * sizeof(CGSize));
//    CTFontGetAdvancesForGlyphs(font, kCTFontOrientationDefault, glyphs, advances, nChars);
//    
//    // Extract kerns
//    CFDataRef kernTable = CTFontCopyTable(font, kCTFontTableKern, 0);
//    u8 *kernBytes = malloc(CFDataGetLength(kernTable));
//    CFDataGetBytes(kernTable, CFRangeMake(0, CFDataGetLength(kernTable)), kernBytes);
//    
//    SimpleFontHeader *fontHeader = (SimpleFontHeader *)kernBytes;
//    Assert(CFSwapInt32BigToHost(fontHeader->tableHeader.version) == 0x00010000);
//    Assert(CFSwapInt32BigToHost(fontHeader->tableHeader.nTables) == 1);
//    const u16 coverageKernFormatMask = 0xFF;
//    Assert((CFSwapInt16BigToHost(fontHeader->subTableHeader.coverage) & coverageKernFormatMask) == 0);
//    
//    u16 nPairs = CFSwapInt16BigToHost(fontHeader->format0Header.nPairs);
//    
//    KerningPair *pairs = (KerningPair *)(kernBytes + sizeof(SimpleFontHeader));
//    
//    for (u32 i = 0; i < nPairs; ++i) {
//        KerningPair pair = {
//            CFSwapInt16BigToHost(pairs[i].l),
//            CFSwapInt16BigToHost(pairs[i].r),
//            (s16)CFSwapInt16BigToHost(pairs[i].val)
//        };
//        NSLog(@"%d, %d, %d", pair.l, pair.r, pair.val);
//    }
//}

void generateMipmaps() {
    const u32 firstMipSize = 512;
    for (u32 i = firstMipSize; i > 0; i /= 2) {
        NSString *filename = [NSString stringWithFormat:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/mipmap-images/blocks-atlas-%d.png", i];
        NSImage *nsImage = [[NSImage alloc] initWithContentsOfFile:filename];
        NSImageRep *imageRep = nsImage.representations[0];
        u32 inWidth = (u32)imageRep.pixelsWide;
        u32 inHeight = (u32)imageRep.pixelsHigh;

        CGImageRef image = [nsImage CGImageForProposedRect:nil context:nil hints:nil];

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
        CGContextRef ctx = CGBitmapContextCreate(NULL, inWidth, inHeight, 8, inWidth, colorSpace, kCGImageAlphaNone);

        CGRect rect = CGRectMake(0, 0, inWidth, inHeight);
        CGContextDrawImage(ctx, rect, image);

        u8 *texels = (u8 *)CGBitmapContextGetData(ctx);
        
        NSData *pixelData = [NSData dataWithBytes:texels length:inWidth * inHeight];
        NSString *outFilename = [NSString stringWithFormat:@"/Users/seanhickey/Projects/IMBlocks/Shared/Textures/blocks-mip-%d.dat", i];
        [pixelData writeToFile:outFilename atomically:YES];
    }
    
}

void generateSdfTexture() {
            
    NSImage *nsImage = [[NSImage alloc] initWithContentsOfFile:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/blocks-atlas.png"];
    NSImageRep *imageRep = nsImage.representations[0];
    u32 inWidth = (u32)imageRep.pixelsWide;
    u32 inHeight = (u32)imageRep.pixelsHigh;

    assert(inWidth == inHeight);

    u32 outWidth = 512;
    u32 outHeight = 512;

    CGImageRef image = [nsImage CGImageForProposedRect:nil context:nil hints:nil];

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx = CGBitmapContextCreate(NULL, inWidth, inHeight, 8, inWidth, colorSpace, kCGImageAlphaNone);

    CGRect rect = CGRectMake(0, 0, inWidth, inHeight);
    CGContextDrawImage(ctx, rect, image);

    u8 *pixels = (u8 *)CGBitmapContextGetData(ctx);

    f32 *sdfRep = deadReckoning(pixels, inWidth, inHeight);

    // Downsample 
    assert(inHeight % outHeight == 0 && inWidth % outWidth == 0 && outHeight == outWidth);
    f32 *downsampled = malloc(outWidth * outHeight * sizeof(f32));
    u32 stride = inHeight / outHeight;
    for (u32 y = 0; y < inHeight; y += stride) {
        for (u32 x = 0; x < inWidth; x += stride) {
            
            // Blend the square region to the right and down from each `stride`-th pixel into the downsampled buffer
            f32 total = 0;
            for (u32 dy = 0; dy < stride; ++dy) {
                for (u32 dx = 0; dx < stride; ++dx) {
                    total += sdfRep[(y + dy) * inWidth + (x + dx)];
                }
            }
            downsampled[(y / stride) * outWidth + (x / stride)] = total / (f32)(stride * stride);
        }
    }

    // Clamp, normalize, convert to u8s
    const f32 scale = 40.0f;
    u8 *outPixels = malloc(outWidth * outHeight);
    for (u32 y = 0; y < outHeight; ++y) {
        for (u32 x = 0; x < outWidth; ++x) {
            f32 val = downsampled[y * outWidth + x];
            f32 clamped = clamp(val, -scale, scale); // Now in interval [-scale, scale]
            f32 normalized = clamped / scale; // Now in interval [-1, 1]
            f32 newVal = ((normalized + 1.0f) / 2.0f) * UINT8_MAX; // Now uint in range [0...UINT8_MAX]
            outPixels[y * outWidth + x] = newVal;
        }
    }

    NSData *pixelData = [NSData dataWithBytes:outPixels length:outWidth * outHeight];
    [pixelData writeToFile:@"/Users/seanhickey/Projects/IMBlocks/Shared/Textures/blocks-atlas.dat" atomically:YES];

    #if OUTPUT_SDF_IMAGE
    CGContextRef outCtx = CGBitmapContextCreate(outPixels, outWidth, outHeight, 8, outWidth, colorSpace, kCGImageAlphaNone);
    CGImageRef outImage = CGBitmapContextCreateImage(outCtx);

    CFURLRef url = (__bridge CFURLRef)[NSURL fileURLWithPath:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/blocks-atlas-sdf.png"];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, nil);
    CGImageDestinationAddImage(dest, outImage, nil);
    CGImageDestinationFinalize(dest);
    #endif
}

f32 *deadReckoning(u8 *pixels, u32 width, u32 height) {
    //
    // Dead-reckoning algorithm via: https://perso.ensta-paris.fr/~manzaner/Download/IAD/Grevera_04.pdf
    //
    
    f64 distAdj  = 1.0;
    f64 distDiag = sqrt(2.0);
    
    u32 numPixels = (u32)width * (u32)height;
    
    f32 *outPixels = (f32 *)malloc(numPixels * sizeof(f32));
    Pt *borderPoints = (Pt *)malloc(numPixels * sizeof(Pt));
    
    #define pix(x, y) pixels[((y) * width) + (x)]
    #define outPix(x, y) outPixels[((y) * width) + (x)]
    #define border(x, y) borderPoints[((y) * width) + (x)]
    #define distForm(x, y, p) sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y))
    
    // Init output data structs
    for(u32 i = 0; i < width * height; ++i) {
        outPixels[i] = width * height; // Big number
    }
    memset(borderPoints, 0xFF, numPixels * sizeof(Pt));
    
    // Init points that are exactly along the boundary of an object
    for (u32 y = 1; y < height - 1; ++y) {
        for (u32 x = 1; x < width - 1; ++x) {
            u8 p = pix(x, y);
            if (pix(x - 1, y) != p ||
                pix(x + 1, y) != p ||
                pix(x, y - 1) != p ||
                pix(x, y + 1) != p) {
                
                outPix(x, y) = 0;
                border(x, y).x = x;
                border(x, y).y = y;
            }
        }
    }
    
    // Forward pass
    for (u32 y = 1; y < height - 2; ++y) {
        for (u32 x = 1; x < width - 2; ++x) {
            if (outPix(x - 1, y - 1) + distDiag < outPix(x, y)) {
                border(x, y) = border(x - 1, y - 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x, y - 1) + distAdj < outPix(x, y)) {
                border(x, y) = border(x, y - 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x + 1, y - 1) + distDiag < outPix(x, y)) {
                border(x, y) = border(x + 1, y - 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x - 1, y) + distAdj < outPix(x, y)) {
                border(x, y) = border(x - 1, y);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
        }
    }
    
    // Reverse pass
    for (u32 y = height - 2; y > 0; --y) {
        for (u32 x = width - 2; x > 0; --x) {
            if (outPix(x + 1, y) + distAdj < outPix(x, y)) {
                border(x, y) = border(x + 1, y);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x - 1, y + 1) + distDiag < outPix(x, y)) {
                border(x, y) = border(x - 1, y + 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x, y + 1) + distAdj < outPix(x, y)) {
                border(x, y) = border(x, y + 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
            if (outPix(x + 1, y + 1) + distDiag < outPix(x, y)) {
                border(x, y) = border(x + 1, y + 1);
                Pt p = border(x, y);
                outPix(x, y) = distForm(x, y, p);
            }
        }
    }
    
    // Flip sign pixels that are outside any object
    for (u32 idx = 0; idx < width * height; ++idx) {
        if (pixels[idx] == 0) {
            outPixels[idx] *= -1.0;
        }
    }
    
    return outPixels;
}
