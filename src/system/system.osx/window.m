#import "window.h"
#import <events/events.h>
#import <time/time.h>

static uint16_t key_modifiers_for_event(NSEvent *event);
static void snow_osx_handle_event(NSWindow *window, NSEvent *event);

@implementation SnowWindow

@synthesize context = _context;

- (void)sendEvent:(NSEvent *)event
{
  snow_osx_handle_event(self, event);

  switch (event.type) {
  case NSKeyDown:
  case NSKeyUp:
    return;

  default:
    break;
  }

  [super sendEvent:event];
}

- (BOOL)windowShouldClose:(id)sender
{
  event_t event = {
    .sender = (__bridge void *)self,
    .time = current_time(),
    .kind = EVENT_WINDOW_CLOSE
  };
  com_queue_event(event);
  return NO;
}

- (void)windowDidResize:(NSNotification *)notify
{
  NSRect frame = [self frame];

  event_t event = {
    .sender = (__bridge void *)self,
    .time = current_time(),
    .kind = EVENT_WINDOW_RESIZE,
    .resize = {
      .x = (int32_t)frame.origin.x,
      .y = (int32_t)frame.origin.y,
      .width = (int32_t)frame.size.width,
      .height = (int32_t)frame.size.height,
    }
  };

  [_context update];

  com_queue_event(event);
}

@end


#pragma mark Event handling

// Get key modifiers for the NSEvent
static uint16_t key_modifiers_for_event(NSEvent *event)
{
  NSUInteger modifiers = event.modifierFlags;
  uint16_t result = MOD_NONE;

  if (modifiers & NSShiftKeyMask) result |= MOD_SHIFT;
  if (modifiers & NSControlKeyMask) result |= MOD_CTRL;
  if (modifiers & NSAlternateKeyMask) result |= MOD_OPT;
  if (modifiers & NSCommandKeyMask) result |= MOD_CMD|MOD_META;

  return result;
}

// Handle the NSEvent
// Takes a sender (the window) and queues the event in the event queue.
static void snow_osx_handle_event(NSWindow *window, NSEvent *event)
{
  event_t msg;

  msg.sender = (__bridge void *)window;
  msg.time = current_time();

  switch (event.type) {

  case NSKeyDown:
    msg.kind = EVENT_KEYBOARD;
    msg.key.pressed = 1;
    break;

  case NSKeyUp:
    msg.kind = EVENT_KEYBOARD;
    msg.key.pressed = 0;
    break;

  case NSLeftMouseDown:
  case NSRightMouseDown:
    msg.kind = EVENT_MOUSE_BUTTON;
    msg.mouse.pressed = 1;
    break;

  case NSLeftMouseUp:
  case NSRightMouseUp:
    msg.kind = EVENT_MOUSE_BUTTON;
    msg.mouse.pressed = 0;
    break;

    default:
    return;
  }

  if (msg.kind == EVENT_MOUSE_BUTTON) {
    NSPoint screen_loc = event.locationInWindow;
    msg.mouse.button = event.buttonNumber;
    if (window && [window isKindOfClass:[NSWindow class]]) {
      screen_loc = [window.contentView
        convertPoint:screen_loc
        fromView:nil];
    }
    msg.mouse.position[0] = (s_float_t)screen_loc.x;
    msg.mouse.position[1] = (s_float_t)screen_loc.y;
  }

  if (msg.kind == EVENT_KEYBOARD) {
    msg.key.key = event.keyCode;
    msg.key.modifiers = key_modifiers_for_event(event);
    msg.key.is_a_repeat = (uint8_t)event.isARepeat;

    NSString *char_string = [event characters];
    const uint16_t *chars = (uint16_t *)
      [char_string cStringUsingEncoding:NSUTF16StringEncoding];
    const size_t count = [char_string length];
    size_t index = 0;

    for (; index < count; ++index) {
      event_t disp_msg = msg;
      disp_msg.key.character = chars[index];
      com_queue_event(disp_msg);
    }
  } else {
    com_queue_event(msg);
  }
}
