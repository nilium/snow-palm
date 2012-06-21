#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#import <snow-config.h>
#import <time/time.h>

#import "NSOpenGLPixelFormat+SnowPF.h"
#import "window.h"

#import "../system.h"

#import "../sgl.h"

#import "app_delegate.h"

#import <memory/allocator.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static CGDisplayModeRef g_default_displaymode = NULL;
static NSString *g_osx_app_title = @APP_TITLE;
static NSOpenGLContext *g_gl_context = nil;
static CVDisplayLinkRef g_display_link = NULL;
static s_time_t g_previous_time = 0;
static s_time_t g_time_frequency = 1;
static s_time_t g_frame_time = 1.0 / 60.0;


///// Prototypes
// Frame callback (calls into sys_frame)
static CVReturn link_frame_callback(
   CVDisplayLinkRef displayLink,
   const CVTimeStamp *inNow,
   const CVTimeStamp *inOutputTime,
   CVOptionFlags flagsIn,
   CVOptionFlags *flagsOut,
   void *displayLinkContext);

// Window destruction (atexit)
static void reset_display_atexit(void);

// Window creation
static void create_fullscreen_window(
  NSApplication *app,
  NSOpenGLContext *ctx,
  const size_t width,
  const size_t height,
  const size_t bpp);

static void create_normal_window(
  NSApplication *app,
  NSOpenGLContext *ctx,
  const size_t width,
  const size_t height);

// Application menu
static void init_menu(NSApplication *app);

// Display mode prototypes
static size_t bpp_for_mode(CGDisplayModeRef mode);
static size_t score_for_dimension(const size_t dim, const size_t mode_dim);

static size_t score_for_mode_params(
    CGDisplayModeRef mode,
    const size_t width,
    const size_t height,
    const size_t bpp);

static CGDisplayModeRef display_mode_for_params(
    const size_t width,
    const size_t height,
    const size_t bpp,
    const CGDirectDisplayID display);


#pragma mark Frame handling callback (display link)

static CVReturn link_frame_callback(
   CVDisplayLinkRef displayLink,
   const CVTimeStamp *inNow,
   const CVTimeStamp *inOutputTime,
   CVOptionFlags flagsIn,
   CVOptionFlags *flagsOut,
   void *displayLinkContext)
{
  s_time_t time = (s_time_t)inOutputTime->hostTime / g_time_frequency;

  if (time < (g_previous_time + g_frame_time))
    return kCVReturnSuccess;

  [g_gl_context makeCurrentContext];

  sys_frame(time);
  g_previous_time = time;

  [g_gl_context flushBuffer];

  return kCVReturnSuccess;
}


#pragma mark Display modes

static size_t bpp_for_mode(CGDisplayModeRef mode) {
  NSString *encoding = (__bridge_transfer NSString *)
    CGDisplayModeCopyPixelEncoding(mode);
  const NSStringCompareOptions opt = NSCaseInsensitiveSearch;

  if ([encoding compare:@IO32BitDirectPixels options:opt] == NSOrderedSame)
    return 32;
  else if ([encoding compare:@IO16BitDirectPixels options:opt] == NSOrderedSame)
    return 16;
  else if ([encoding compare:@IO8BitIndexedPixels options:opt] == NSOrderedSame)
    return 8;

  return 0;
}

static size_t score_for_dimension(const size_t dim, const size_t mode_dim) {
  if (mode_dim == dim)
    return 100;
  else if (mode_dim < dim)
    return 25;
  else
    return 50;
}

static size_t score_for_mode_params(
    CGDisplayModeRef mode,
    const size_t width,
    const size_t height,
    const size_t bpp)
{
  const size_t m_bpp = bpp_for_mode(mode);
  const size_t m_width = CGDisplayModeGetWidth(mode);
  const size_t m_height = CGDisplayModeGetHeight(mode);
  const s_float_t aspect = 100 * ((s_float_t)width / (s_float_t)height);
  const s_float_t m_aspect = 100 * ((s_float_t)m_width / (s_float_t)m_height);

  size_t score = 0;

  score += score_for_dimension(width, m_width);
  score += score_for_dimension(height, m_height);

  if (float_equals(m_aspect, aspect))
    score += 100;

  if (m_bpp == bpp)
    score += 100;

  return score;
}

static CGDisplayModeRef display_mode_for_params(
    const size_t width,
    const size_t height,
    const size_t bpp,
    const CGDirectDisplayID display)
{
  NSArray *modes = (__bridge_transfer NSArray *)
    CGDisplayCopyAllDisplayModes(display, NULL);

  CGDisplayModeRef pref_mode = NULL;
  size_t pref_mode_score = 0;

  for (__unsafe_unretained id anon_mode in modes) {
    CGDisplayModeRef mode = (__bridge CGDisplayModeRef)anon_mode;
    size_t score = score_for_mode_params(mode, width, height, bpp);

    if (score == 400) {
      pref_mode = mode;
      break;
    } else if (score > pref_mode_score) {
      pref_mode_score = score;
      pref_mode = mode;
    }
  }

  if (pref_mode != NULL)
    CGDisplayModeRetain(pref_mode);

  return pref_mode;
}


#pragma mark Window creation

// Fullscreen exit handling
static void reset_display_atexit(void) {
  if (g_default_displaymode != NULL) {
    // do things if fullscreen
  }
}

// Create a fullscreen window
static void create_fullscreen_window(
  NSApplication *app,
  NSOpenGLContext *ctx,
  const size_t width,
  const size_t height,
  const size_t bpp)
{
  (void)app;

  CGDirectDisplayID const display = CGMainDisplayID();
  CGDisplayModeRef best_mode = display_mode_for_params(800, 600, 32, display);
  g_default_displaymode = CGDisplayCopyDisplayMode(display);

  if (best_mode == NULL)
    s_fatal_error(1, "Unable to get adequate display mode for main display");

  s_fatal_error(1, "Not implemented");
}

// Create a windowed mode window
static void create_normal_window(
  NSApplication *app,
  NSOpenGLContext *ctx,
  const size_t width,
  const size_t height)
{
  const NSUInteger style_mask =
    NSTitledWindowMask
  | NSClosableWindowMask
  | NSMiniaturizableWindowMask;

  SnowWindow *window = [[SnowWindow alloc]
    initWithContentRect:NSMakeRect(0, 0, width, height)
    styleMask:style_mask
    backing:NSBackingStoreBuffered
    defer:NO
    screen:nil];

  window.delegate = window;
  window.acceptsMouseMovedEvents = YES;
  window.title = g_osx_app_title;
  [window center];
  [window makeKeyAndOrderFront:app];

  ctx.view = window.contentView;
}

// Create window interface (public)
void sys_create_window(
  NSApplication *app,
  const size_t width,
  const size_t height,
  const size_t bpp,
  const bool fullscreen)
{
  GLint swapInterval = 1;
  NSOpenGLPixelFormat *format =
    [NSOpenGLPixelFormat pixelFormatForWidth:width height:height
      colorSize:bpp depthSize:16 stencilSize:8
      fullScreen:(BOOL)fullscreen];
  NSOpenGLContext *ctx = [[NSOpenGLContext alloc]
    initWithFormat:format shareContext:nil];

  g_gl_context = ctx;

  [ctx makeCurrentContext];
  [ctx setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

  if (fullscreen)
    create_fullscreen_window(app, ctx, width, height, bpp);
  else
    create_normal_window(app, ctx, width, height);

  [ctx update];

  sys_init();

  CGLPixelFormatObj cglFormat = (CGLPixelFormatObj)[format CGLPixelFormatObj];
  CGLContextObj cglContext = (CGLContextObj)[ctx CGLContextObj];

  g_time_frequency = (s_float_t)CVGetHostClockFrequency();
  // setup the frame loop, basically
  CVDisplayLinkCreateWithActiveCGDisplays(&g_display_link);
  CVDisplayLinkSetOutputCallback(g_display_link, link_frame_callback, NULL);
  CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(g_display_link, cglContext, cglFormat);
  CVDisplayLinkStart(g_display_link);

  s_log_note("Window created");
}


void sys_destroy_window() {
  if (g_display_link != NULL) {
    CVDisplayLinkRelease(g_display_link);
    g_display_link = NULL;

    g_gl_context.view = nil;
    [g_gl_context update];
  }
}


#pragma mark Application menu

// Create the applications's menu
static void init_menu(NSApplication *app) {
  NSMenu *app_menu;
  NSMenu *main_menu;
  NSMenuItem *item;

  main_menu = [NSMenu new];
  app_menu = [NSMenu new];

  NSString *hide_string = [@"Hide " stringByAppendingString:g_osx_app_title];
  NSString *quit_string = [@"Quit " stringByAppendingString:g_osx_app_title];

  [app_menu addItemWithTitle:hide_string action:@selector(hide:) keyEquivalent:@"h"];
  item = [app_menu addItemWithTitle:@"Hide Others"
    action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
  [item setKeyEquivalentModifierMask:NSAlternateKeyMask | NSCommandKeyMask];
  [app_menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:)
    keyEquivalent:@""];

  [app_menu addItem:[NSMenuItem separatorItem]];

  [app_menu addItemWithTitle:quit_string action:@selector(terminate:) keyEquivalent:@"q"];

  item = [NSMenuItem new];
  [item setSubmenu:app_menu];
  [main_menu addItem:item];

  [app setMainMenu:main_menu];
}


#pragma mark Main routine (sys_main)

void sys_main(int argc, const char *argv[]) {
  @autoreleasepool {
    allocator_t *alloc = g_default_allocator;
    NSApplication *app;
    NSBundle *bundle;
    char *base_dir;
    const char *pref_dir;
    const char *ph_error;

    app = [NSApplication sharedApplication];

    SnowAppDelegate *delegate = [SnowAppDelegate new];
    [app setDelegate:delegate];

    pref_dir = PHYSFS_getPrefDir(APP_ORGANIZATION, APP_TITLE);

    bundle = [NSBundle mainBundle];
    if (bundle != nil) {
      NSString *resPath;
      NSUInteger length_encoded;

      resPath = bundle.resourcePath;
      resPath = [resPath stringByAppendingString: @"/" APP_BASE_DIR];

      length_encoded = [resPath lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
      base_dir = com_malloc(alloc, (size_t)length_encoded + 1);

      [resPath getCString:base_dir
                maxLength:(length_encoded + 1)
                 encoding:NSUTF8StringEncoding];
    } else {
      const char *last_slash;
      size_t mount_len;
      size_t base_len;

      last_slash = strrchr(argv[0], '/');

      if (!last_slash) {
        printf("Failed to initialize filesystem");
        return;
      }

      base_len = strlen(APP_BASE_DIR);
      mount_len = (size_t)(last_slash - argv[0]) + 1;
      // 2 for the NULL and trailing slash
      base_dir = com_malloc(alloc, sizeof(char) * (mount_len + base_len + 2));
      strncpy(base_dir, argv[0], mount_len);
      strncat(base_dir, APP_BASE_DIR, base_len);
    }

    s_log_note("Setting write path to <%s>", pref_dir);
    s_log_note("Setting base path to <%s>", base_dir);

    if ( ! PHYSFS_setWriteDir(pref_dir)) {
      ph_error = PHYSFS_getLastError();
      s_log_error("Failed to set write directory:\n  -> %s", ph_error);
    }

    if ( ! PHYSFS_mount(pref_dir, NULL, TRUE)) {
      ph_error = PHYSFS_getLastError();
      s_log_error("Failed to mount write directory:\n  -> %s", ph_error);
    }

    if ( ! PHYSFS_mount(base_dir, NULL, TRUE)) {
      ph_error = PHYSFS_getLastError();
      s_log_error("Failed to mount base directory:\n  -> %s", ph_error);
    }

    com_free(base_dir);

    init_menu(app);
    sys_create_window(app, 800, 600, 32, false);

    [app run];
  }
}


#pragma mark Application termination

void sys_terminate() {
  s_log_note("sys_terminate called");
  dispatch_async(dispatch_get_main_queue(), ^{
    s_log_note("sys_terminate executing");
    NSApplication *app = [NSApplication sharedApplication];
    [app terminate:app];
  });
}

#ifdef __cplusplus
}
#endif // __cplusplus
