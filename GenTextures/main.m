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

typedef struct Pt {
    s32 x;
    s32 y;
} Pt;

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        NSImage *nsImage = [[NSImage alloc] initWithContentsOfFile:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/block-textures.png"];
        u32 width = nsImage.size.width;
        u32 height = nsImage.size.height;
        u32 numPixels = (u32)width * (u32)height;

        CGImageRef image = [nsImage CGImageForProposedRect:nil context:nil hints:nil];
        
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
        CGContextRef ctx = CGBitmapContextCreate(NULL, width, height, 8, width, colorSpace, kCGImageAlphaNone);
        
        CGRect rect = CGRectMake(0, 0, width, height);
        CGContextDrawImage(ctx, rect, image);
        
        u8 *pixels = (u8 *)CGBitmapContextGetData(ctx);
        
        //
        // Dead-reckoning algorithm via: https://perso.ensta-paris.fr/~manzaner/Download/IAD/Grevera_04.pdf
        //
        
        f64 distAdj  = 1.0;
        f64 distDiag = sqrt(2.0);
        
        s8 *outPixels = (s8 *)malloc(numPixels);
        Pt *borderPoints = (Pt *)malloc(numPixels * sizeof(Pt));
        
        #define pix(x, y) pixels[((y) * width) + (x)]
        #define outPix(x, y) outPixels[((y) * width) + (x)]
        #define border(x, y) borderPoints[((y) * width) + (x)]
        #define distForm(x, y, p) sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y))
        
        // Init output data structs
        memset(outPixels, INT8_MAX, numPixels);
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
        
        // Convert back in u8s
        for (u32 idx = 0; idx < width * height; ++idx) {
            s16 p = (s16)outPixels[idx];
            outPixels[idx] = (u8)(p + 127);
        }
        
        NSData *pixelData = [NSData dataWithBytes:outPixels length:numPixels];
        [pixelData writeToFile:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/block-textures.dat" atomically:YES];
        
        CGContextRef outCtx = CGBitmapContextCreate(outPixels, width, height, 8, width, colorSpace, kCGImageAlphaNone);
        CGImageRef outImage = CGBitmapContextCreateImage(outCtx);
        
        CFURLRef url = (__bridge CFURLRef)[NSURL fileURLWithPath:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/block-textures-CONVERTED.png"];
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, nil);
        CGImageDestinationAddImage(dest, outImage, nil);
        CGImageDestinationFinalize(dest);
    }
    return 0;
}
