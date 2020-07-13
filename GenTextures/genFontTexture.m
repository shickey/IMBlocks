//
//  genFontTexture.c
//  GenTextures
//
//  Created by Sean Hickey on 7/10/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#include "genFontTexture.h"
#import <Foundation/Foundation.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

int stbtt_PackFontRangesGatherSdfRects(stbtt_pack_context *spc, const stbtt_fontinfo *info, float scale, int padding, stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects)
{
   int i,j,k;
   int missing_glyph_added = 0;

   k=0;
   for (i=0; i < num_ranges; ++i) {
      float fh = ranges[i].font_size;
      float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
//      ranges[i].h_oversample = (unsigned char) spc->h_oversample;
//      ranges[i].v_oversample = (unsigned char) spc->v_oversample;
      for (j=0; j < ranges[i].num_chars; ++j) {
         int x0,y0,x1,y1;
         int codepoint = ranges[i].array_of_unicode_codepoints == NULL ? ranges[i].first_unicode_codepoint_in_range + j : ranges[i].array_of_unicode_codepoints[j];
         int glyph = stbtt_FindGlyphIndex(info, codepoint);
         if (glyph == 0 && (spc->skip_missing || missing_glyph_added)) {
            rects[k].w = rects[k].h = 0;
         } else {
            stbtt_GetGlyphBitmapBoxSubpixel(info,glyph,
                                            scale,
                                            scale,
                                            0,0,
                                            &x0,&y0,&x1,&y1);
            rects[k].w = (stbrp_coord) (x1-x0 + padding * 2);
            rects[k].h = (stbrp_coord) (y1-y0 + padding * 2);
            if (glyph == 0)
               missing_glyph_added = 1;
         }
         ++k;
      }
   }

   return k;
}

int stbtt_PackFontRangesRenderSdfIntoRects(stbtt_pack_context *spc, const stbtt_fontinfo *info, float scale, int padding, stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects)
{
   int i,j,k, missing_glyph = -1, return_value = 1;

   // save current values
   int old_h_over = spc->h_oversample;
   int old_v_over = spc->v_oversample;

   k = 0;
   for (i=0; i < num_ranges; ++i) {
      float fh = ranges[i].font_size;
      float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
//      float recip_h,recip_v,sub_x,sub_y;
//      spc->h_oversample = ranges[i].h_oversample;
//      spc->v_oversample = ranges[i].v_oversample;
//      recip_h = 1.0f / spc->h_oversample;
//      recip_v = 1.0f / spc->v_oversample;
//      sub_x = stbtt__oversample_shift(spc->h_oversample);
//      sub_y = stbtt__oversample_shift(spc->v_oversample);
      for (j=0; j < ranges[i].num_chars; ++j) {
         stbrp_rect *r = &rects[k];
         if (r->was_packed && r->w != 0 && r->h != 0) {
            stbtt_packedchar *bc = &ranges[i].chardata_for_range[j];
            int advance, lsb, x0,y0,x1,y1;
            int codepoint = ranges[i].array_of_unicode_codepoints == NULL ? ranges[i].first_unicode_codepoint_in_range + j : ranges[i].array_of_unicode_codepoints[j];
            int glyph = stbtt_FindGlyphIndex(info, codepoint);
            stbrp_coord pad = (stbrp_coord) spc->padding;

            // pad on left and top
            r->x += pad;
            r->y += pad;
            r->w -= pad;
            r->h -= pad;
            stbtt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
            stbtt_GetGlyphBitmapBox(info, glyph,
                                    scale,
                                    scale,
                                    &x0,&y0,&x1,&y1);
//            stbtt_MakeGlyphBitmapSubpixel(info,
//                                          spc->pixels + r->x + r->y*spc->stride_in_bytes,
//                                          r->w - spc->h_oversample+1,
//                                          r->h - spc->v_oversample+1,
//                                          spc->stride_in_bytes,
//                                          scale * spc->h_oversample,
//                                          scale * spc->v_oversample,
//                                          0,0,
//                                          glyph);
             
             int glyphBoxW = r->w;
             int glyphBoxH = r->h;
             
             unsigned char onedge_value = 127;
             float pixel_dist_scale = 127.0f / ((float)padding);
             
             int sdfW, sdfH, sdfXOff, sdfYOff;
             unsigned char *glyph_pixels = stbtt_GetGlyphSDF(info, scale, glyph, padding, onedge_value, pixel_dist_scale, &sdfW, &sdfH, &sdfXOff, &sdfYOff);
             
             if (glyph_pixels) {
                 unsigned char *start = spc->pixels + r->x + r->y*spc->stride_in_bytes;
                 for (u32 i = 0; i < glyphBoxH; ++i) {
                     unsigned char *inputRowStart = glyph_pixels + (sdfW * i);
                     unsigned char *outputRowStart = start + (spc->stride_in_bytes * i);
                     memcpy(outputRowStart, inputRowStart, glyphBoxW);
                 }
             }
             
             

//            if (spc->h_oversample > 1)
//               stbtt__h_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
//                                  r->w, r->h, spc->stride_in_bytes,
//                                  spc->h_oversample);
//
//            if (spc->v_oversample > 1)
//               stbtt__v_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
//                                  r->w, r->h, spc->stride_in_bytes,
//                                  spc->v_oversample);

            bc->x0       = (stbtt_int16)  r->x;
            bc->y0       = (stbtt_int16)  r->y;
            bc->x1       = (stbtt_int16) (r->x + r->w);
            bc->y1       = (stbtt_int16) (r->y + r->h);
            bc->xadvance =                scale * advance;
             bc->xoff     =       sdfXOff;
             bc->yoff     =       sdfYOff;
//            bc->xoff     =       (float)  x0 * recip_h + sub_x;
//            bc->yoff     =       (float)  y0 * recip_v + sub_y;
//            bc->xoff2    =                (x0 + r->w) * recip_h + sub_x;
//            bc->yoff2    =                (y0 + r->h) * recip_v + sub_y;

            if (glyph == 0)
               missing_glyph = j;
         } else if (spc->skip_missing) {
            return_value = 0;
         } else if (r->was_packed && r->w == 0 && r->h == 0 && missing_glyph >= 0) {
            ranges[i].chardata_for_range[j] = ranges[i].chardata_for_range[missing_glyph];
         } else {
            return_value = 0; // if any fail, report failure
         }

         ++k;
      }
   }

   // restore original values
   spc->h_oversample = old_h_over;
   spc->v_oversample = old_v_over;

   return return_value;
}

int stbtt_PackSdfFontRanges(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index, float scale, int padding, stbtt_pack_range *ranges, int num_ranges)
{
   stbtt_fontinfo info;
   int i,j,n, return_value = 1;
   //stbrp_context *context = (stbrp_context *) spc->pack_info;
   stbrp_rect    *rects;

   // flag all characters as NOT packed
   for (i=0; i < num_ranges; ++i)
      for (j=0; j < ranges[i].num_chars; ++j)
         ranges[i].chardata_for_range[j].x0 =
         ranges[i].chardata_for_range[j].y0 =
         ranges[i].chardata_for_range[j].x1 =
         ranges[i].chardata_for_range[j].y1 = 0;

   n = 0;
   for (i=0; i < num_ranges; ++i)
      n += ranges[i].num_chars;

   rects = (stbrp_rect *) STBTT_malloc(sizeof(*rects) * n, spc->user_allocator_context);
   if (rects == NULL)
      return 0;

   info.userdata = spc->user_allocator_context;
   stbtt_InitFont(&info, fontdata, stbtt_GetFontOffsetForIndex(fontdata,font_index));

//   n = stbtt_PackFontRangesGatherRects(spc, &info, ranges, num_ranges, rects);
    n = stbtt_PackFontRangesGatherSdfRects(spc, &info, scale, padding, ranges, num_ranges, rects);

   stbtt_PackFontRangesPackRects(spc, rects, n);

    return_value = stbtt_PackFontRangesRenderSdfIntoRects(spc, &info, scale, padding, ranges, num_ranges, rects);
//   return_value = stbtt_PackFontRangesRenderIntoRects(spc, &info, ranges, num_ranges, rects);

   STBTT_free(rects, spc->user_allocator_context);
   return return_value;
}

int stbtt_PackSdfFontRange(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index, float font_size, float scale, int padding,
            int first_unicode_codepoint_in_range, int num_chars_in_range, stbtt_packedchar *chardata_for_range)
{
   stbtt_pack_range range;
   range.first_unicode_codepoint_in_range = first_unicode_codepoint_in_range;
   range.array_of_unicode_codepoints = NULL;
   range.num_chars                   = num_chars_in_range;
   range.chardata_for_range          = chardata_for_range;
   range.font_size                   = font_size;
   return stbtt_PackSdfFontRanges(spc, fontdata, font_index, scale, padding, &range, 1);
}

void generateStbFont() {
    NSData *fontData = [NSData dataWithContentsOfFile:@"/System/Library/Fonts/HelveticaNeue.ttc"];
    u8 *fontBytes = (u8 *)fontData.bytes;
    
    int fontIdx = 1; // I *think* this is the Bold variant
    
    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontBytes, fontIdx);
    
    stbtt_pack_context packCtx;
    const u32 TEX_WIDTH = 512;
    const u32 TEX_HEIGHT = 512;
    u8 *pixels = (u8 *)malloc(TEX_WIDTH * TEX_HEIGHT);
    stbtt_PackBegin(&packCtx, pixels, TEX_WIDTH, TEX_HEIGHT, TEX_WIDTH, 1, NULL);
    u32 numChars = 127 - 32;
    stbtt_packedchar *packedChars = malloc(numChars * sizeof(stbtt_packedchar));
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 48.0);
    stbtt_PackSdfFontRange(&packCtx, fontBytes, fontIdx, 48.0, scale, 4, 32, numChars, packedChars);
    stbtt_PackEnd(&packCtx);
    
    // Write out the image data
    NSData *pixelData = [NSData dataWithBytes:pixels length:TEX_WIDTH * TEX_HEIGHT];
    [pixelData writeToFile:@"/Users/seanhickey/Projects/IMBlocks/Shared/Textures/font-atlas.dat" atomically:YES];
    
#if 1
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx = CGBitmapContextCreate(pixels, TEX_WIDTH, TEX_HEIGHT, 8, TEX_WIDTH, colorSpace, kCGImageAlphaNone);
    
    CGImageRef outImage = CGBitmapContextCreateImage(ctx);

    CFURLRef url = (__bridge CFURLRef)[NSURL fileURLWithPath:@"/Users/seanhickey/Projects/IMBlocks/GenTextures/font-atlas.png"];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, nil);
    CGImageDestinationAddImage(dest, outImage, nil);
    CGImageDestinationFinalize(dest);
#endif
    
    NSMutableString *fontCHeaderOut = [NSMutableString stringWithString:
@"\n"
"struct SdfFontChar {\n"
"    int x0;\n"
"    int y0;\n"
"    int x1;\n"
"    int y1;\n"
"    int w;\n"
"    int h;\n"
"    float xOffset;\n"
"    float yOffset;\n"
"    float advance;\n"
"};\n"
"\n\n"];
    
    [fontCHeaderOut appendFormat:@"static SdfFontChar FONT_DATA[%i] = {\n", numChars + 32];
    for (u32 i = 0; i < 32; ++i) {
        [fontCHeaderOut appendString:@"    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },\n"];
    }
    
    for (u32 i = 0; i < numChars; ++i) {
        stbtt_packedchar packedChar = packedChars[i];
        [fontCHeaderOut appendFormat:@"    { %d, %d, %d, %d, %d, %d, %0.8ff, %0.8ff, %0.8ff },\n", packedChar.x0, 
                                                                                             packedChar.y0,
                                                                                             packedChar.x1, 
                                                                                             packedChar.y1,
                                                                                             packedChar.x1 - packedChar.x0,
                                                                                             packedChar.y1 - packedChar.y0,
                                                                                             packedChar.xoff,
                                                                                             packedChar.yoff,
                                                                                             packedChar.xadvance];
    }
    
    [fontCHeaderOut appendString:@"};\n\n"];
    
    [fontCHeaderOut writeToFile:@"/Users/seanhickey/Projects/IMBlocks/Blocks/font-atlas.h" atomically:YES encoding:NSUTF8StringEncoding error:nil];
}
