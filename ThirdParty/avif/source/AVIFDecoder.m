//
//  AVIFDecoder.m
//  AVIFQuickLook
//
//  Created by lizhuoli on 2019/4/15.
//  Copyright © 2019 dreampiggy. All rights reserved.
//

#import "AVIFDecoder.h"
#import <Accelerate/Accelerate.h>
#import <AppKit/AppKit.h>
#import <avif/avif.h>

// Convert 8/10/12bit AVIF image into RGBA8888
static avifResult ConvertAvifImagePlanarToRGB(avifImage *avif, uint8_t *outPixels) {
	avifResult ret = AVIF_RESULT_OK;
	BOOL hasAlpha = avif->alphaPlane != NULL;
	avifRGBImage avifRgb = {0};
	avifRGBImageSetDefaults(&avifRgb, avif);
	avifRgb.format = hasAlpha ? AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_RGB;
	avifRgb.depth = 8;
	avifRgb.rowBytes = avif->width * avifRGBImagePixelSize(&avifRgb);
	avifRgb.pixels = malloc(avifRgb.rowBytes * avifRgb.height);
	if(avifRgb.pixels) {
		size_t components = hasAlpha ? 4 : 3;
		ret = avifImageYUVToRGB(avif, &avifRgb);
		if(ret == AVIF_RESULT_OK) {
			for(int j = 0; j < avifRgb.height; ++j) {
				for(int i = 0; i < avifRgb.width; ++i) {
					uint8_t *pixel = &outPixels[components * (i + (j * avifRgb.width))];
					pixel[0] = avifRgb.pixels[i * components + (j * avifRgb.rowBytes) + 0];
					pixel[1] = avifRgb.pixels[i * components + (j * avifRgb.rowBytes) + 1];
					pixel[2] = avifRgb.pixels[i * components + (j * avifRgb.rowBytes) + 2];
					if(hasAlpha) {
						pixel[3] = avifRgb.pixels[i * components + (j * avifRgb.rowBytes) + 3];
					}
				}
			}
		}
		free(avifRgb.pixels);
	} else {
		ret = AVIF_RESULT_OUT_OF_MEMORY;
	}
	return ret;
}

static void FreeImageData(void *info, const void *data, size_t size) {
	free((void *)data);
}

@implementation AVIFDecoder

+ (nullable CGImageRef)createAVIFImageAtPath:(nonnull NSString *)path {
	NSData *data = [NSData dataWithContentsOfFile:path];
	if(!data) {
		return nil;
	}
	if(![AVIFDecoder isAVIFFormatForData:data]) {
		return nil;
	}

	return [AVIFDecoder createAVIFImageWithData:data];
}

+ (nullable CGImageRef)createAVIFImageWithData:(nonnull NSData *)data CF_RETURNS_RETAINED {
	// Decode it
	avifDecoder *decoder = avifDecoderCreate();
	if(!decoder) return nil;
	avifImage *avif = avifImageCreateEmpty();
	if(!avif) {
		avifDecoderDestroy(decoder);
		return nil;
	}
	avifResult result = avifDecoderReadMemory(decoder, avif, [data bytes], [data length]);
	if(result != AVIF_RESULT_OK) {
		avifImageDestroy(avif);
		avifDecoderDestroy(decoder);
		return nil;
	}

	int width = avif->width;
	int height = avif->height;
	BOOL hasAlpha = avif->alphaPlane != NULL;
	size_t components = hasAlpha ? 4 : 3;
	size_t bitsPerComponent = 8;
	size_t bitsPerPixel = components * bitsPerComponent;
	size_t rowBytes = width * bitsPerPixel / 8;

	uint8_t *dest = calloc(width * components * height, sizeof(uint8_t));
	if(!dest) {
		avifImageDestroy(avif);
		avifDecoderDestroy(decoder);
		return nil;
	}
	// convert planar to RGB888/RGBA8888
	result = ConvertAvifImagePlanarToRGB(avif, dest);

	// We don't need these any more
	avifImageDestroy(avif);
	avifDecoderDestroy(decoder);

	if(result != AVIF_RESULT_OK) {
		free(dest);
		return nil;
	}

	// dest is swallowed up by this, and will be freed by FreeImageData
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, dest, rowBytes * height, FreeImageData);
	CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
	bitmapInfo |= hasAlpha ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;
	CGColorSpaceRef colorSpaceRef = [self colorSpaceGetDeviceRGB];
	CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;
	CGImageRef imageRef = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, rowBytes, colorSpaceRef, bitmapInfo, provider, NULL, NO, renderingIntent);

	// clean up
	CGDataProviderRelease(provider);

	return imageRef;
}

#pragma mark - Helper
+ (BOOL)isAVIFFormatForData:(nullable NSData *)data {
	if(!data) {
		return NO;
	}
	if(data.length >= 12) {
		//....ftypavif ....ftypavis
		NSString *testString = [[NSString alloc] initWithData:[data subdataWithRange:NSMakeRange(4, 8)] encoding:NSASCIIStringEncoding];
		if([testString isEqualToString:@"ftypavif"] || [testString isEqualToString:@"ftypavis"]) {
			return YES;
		}
	}

	return NO;
}

+ (CGColorSpaceRef)colorSpaceGetDeviceRGB {
	CGColorSpaceRef screenColorSpace = NSScreen.mainScreen.colorSpace.CGColorSpace;
	if(screenColorSpace) {
		return screenColorSpace;
	}
	static CGColorSpaceRef colorSpace;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		colorSpace = CGColorSpaceCreateDeviceRGB();
	});
	return colorSpace;
}

@end
