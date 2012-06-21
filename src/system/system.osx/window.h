#ifndef __SNOW_WINDOW_H__
#define __SNOW_WINDOW_H__ 1

#import <Cocoa/Cocoa.h>

@interface SnowWindow : NSWindow <NSWindowDelegate> {
}

@property (readwrite, retain) NSOpenGLContext *context;

@end

#endif /* end __WINDOW_HH__ include guard */
