#import "NSOpenGLPixelFormat+SnowPF.h"

@implementation NSOpenGLPixelFormat (SnowPixelFormat)

+ (NSOpenGLPixelFormat *)pixelFormatForWidth:(size_t)width height:(size_t)height
  colorSize:(size_t)bpp depthSize:(size_t)depth stencilSize:(size_t)stencil
  fullScreen:(BOOL)fullscreen
{
  NSOpenGLPixelFormatAttribute attrs[16];
  NSOpenGLPixelFormatAttribute *attr_ptr = attrs;
  NSOpenGLPixelFormat *format = nil;

  *attr_ptr++ = NSOpenGLPFADoubleBuffer;
  *attr_ptr++ = NSOpenGLPFAMaximumPolicy;
  *attr_ptr++ = NSOpenGLPFAClosestPolicy;

  if (fullscreen)
    *attr_ptr++ = NSOpenGLPFAFullScreen;
  
  if (bpp > 0) {
    *attr_ptr++ = NSOpenGLPFAColorSize;
    *attr_ptr++ = bpp;
  }
  
  if (depth > 0) {
    *attr_ptr++ = NSOpenGLPFADepthSize;
    *attr_ptr++ = depth;
  }

  if (stencil > 0) {
    *attr_ptr++ = NSOpenGLPFAStencilSize;
    *attr_ptr++ = stencil;
  }

  // terminate attrs list
  *attr_ptr = 0;

  format = [[self alloc] initWithAttributes:attrs];

  return format;
}

@end
