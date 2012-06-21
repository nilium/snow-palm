#import "app_delegate.h"

#import <system/system.h>
#import <events/events.h>
#import <log/log.h>
#import <time/time.h>

static void queue_active_event(id sender, bool active)
{
  event_t event = {
    .sender = (__bridge void *)sender,
    .time = current_time(),
    .kind = EVENT_WINDOW_ACTIVE,
    .active = {
      .active = active
    }
  };

  com_queue_event(event);
}

@implementation SnowAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)note
{
  [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
  return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app
{
  return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)note
{
  sys_quit();
}

- (void)applicationWillBecomeActive:(NSNotification *)note
{
  queue_active_event(self, true);
}

- (void)applicationWillResignActive:(NSNotification *)note
{
  queue_active_event(self, false);
}

@end
