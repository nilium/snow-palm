#import "app_delegate.h"
#import <log/log.h>

@implementation SnowAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)note {
  s_log_note("App running");
  [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
  s_log_note("Windows closed, terminating app");
  return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app {
  s_log_note("Terminating app now");
  return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)note {
  s_log_note("Application will terminate");
  sys_quit();
}

- (void)dealloc {
  s_log_note("Deallocating delegate");
  [super dealloc];
}

@end
