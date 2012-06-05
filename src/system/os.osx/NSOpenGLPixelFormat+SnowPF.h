#ifndef __SNOW_NSOPENGLPIXELFORMAT_SNOWGLPIXELFORMAT_H__
#define __SNOW_NSOPENGLPIXELFORMAT_SNOWGLPIXELFORMAT_H__ 1

#import <Cocoa/Cocoa.h>

@interface NSOpenGLPixelFormat (SnowGLPixelFormat)

+ (NSOpenGLPixelFormat *)pixelFormatForWidth:(size_t)width height:(size_t)height
  colorSize:(size_t)bpp depthSize:(size_t)depth stencilSize:(size_t)stencil
  fullScreen:(BOOL)fullscreen;

@end

#endif /* end __SNOW_NSOPENGLPIXELFORMAT_SNOWGLPIXELFORMAT_H__ include guard */
