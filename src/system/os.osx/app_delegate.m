#import "app_delegate.h"
#import "../system.h"
#import <log/log.h>

@implementation SnowAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)note {
  [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
  return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app {
  return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)note {
  sys_quit();
}

@end
