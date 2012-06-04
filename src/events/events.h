#ifndef __SNOW_EVENT_H__
#define __SNOW_EVENT_H__ 1

#include <snow-config.h>
#include <maths/maths.h>
#include <memory/allocator.h>

#ifdef __APPLE__
  #ifdef TARGET_OS_MAC
    #import <Cocoa/Cocoa.h>
  #elif defined(TARGET_OS_IPHONE)
    #import <CocoaTouch/CocoaTouch.h>
  #else
    #error "Undefined Apple operating system"
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
  max touches is two.  Rationale: four touches or more sets off iOS's
  multitasking gestures.  five touches is the maximum for most other devices.
  realistically, a gesture using even three touches is pushing it on a
  gesture's complexity, so the best option is to avoid anything requiring
  more than two fingers, which covers all well-known gestures.
*/
#ifndef S_MAX_TOUCHES
#define S_MAX_TOUCHES (2)
#endif // !S_MAX_TOUCHES

typedef enum e_touch_phase {
#if S_PLATFORM_APPLE
  #if S_PLATFORM_IOS || S_PLATFORM_IOS_SIM
  #elif S_PLATFORM_MAC
  TOUCH_BEGAN = NSTouchPhaseBegan,
  TOUCH_MOVED = NSTouchPhaseMoved,
  TOUCH_STATIONARY = NSTouchPhaseStationary,
  TOUCH_ENDED = NSTouchPhaseEnded,
  TOUCH_CANCELLED = NSTouchPhaseCancelled,
  TOUCH_TOUCHING = NSTouchPhaseTouching,
  TOUCH_ANY = NSTouchPhaseAny
  #else
    #error "Undefined Apple operating system"
  #endif
#else
  TOUCH_BEGAN = 1,
  TOUCH_MOVED = 1 << 1,
  TOUCH_STATIONARY = 1 << 2,
  TOUCH_ENDED = 1 << 3,
  TOUCH_CANCELLED = 1 << 4,
  TOUCH_TOUCHING=TOUCH_BEGAN|TOUCH_MOVED|TOUCH_STATIONARY,
  TOUCH_ANY = 0xFFFF
#endif
} touch_phase_t;

typedef enum {
  EVENT_NONE = 0,
  EVENT_KEYBOARD,
  EVENT_TOUCH,
  EVENT_MOUSE_MOTION,
  EVENT_MOUSE_BUTTON,
  EVENT_WINDOW_RESIZE,
  EVENT_RESET_GRAPHCIS,
} event_kind_t;

typedef enum {
  MOD_NONE = 0,
  MOD_SHIFT = 1,         // shift
  MOD_CTRL = 1 << 1,    // control
  MOD_CMD = 1 << 2,     // command
  MOD_OPT = 1 << 3,     // opt/alt
  MOD_META = 1 << 4     // start/meta
} key_modifier_t;

typedef enum {
  MOUSE_LEFT = 1,
  MOUSE_RIGHT = 2,
  MOUSE_MIDDLE = 3
} mouse_button_t;

struct key_event_t {
  uint16_t modifiers;
  uint16_t key;
  uint16_t character;
  uint8_t pressed;
  uint8_t is_a_repeat;
};

struct mouse_button_event_t {
  vec2_t position;
  uint8_t button;
  uint8_t pressed;
};

struct mouse_motion_event_t {
  vec2_t position;
};

struct touch_t {
  vec2_t position;
  touch_phase_t phase;
};

struct touch_event_t {
  touch_t touches[S_MAX_TOUCHES];
  uint8_t num_touches;
};

struct event_t {
  void *sender;
  s_time_t time;
  event_kind_t kind;

  union {
    key_event_t key;
    mouse_button_event_t mouse;
    mouse_motion_event_t motion;
    touch_event_t touch;
  };
};


typedef bool (*event_handler_fn_t)(event_t *event, void *context);


/******************************************************************************
** Event API
******************************************************************************/

// Pass to com_remove_event_handler to ignore a handler's context when removing
// it from the registered event handlers.
#define IGNORE_HANDLER_CONTEXT ((void *)-1)

void sys_init_events(allocator_t *alloc);

// Stores an event in the event queue.
void com_queue_event(event_t event);
void com_process_event_queue(void);
// Emits an event (causes all event handlers to process it until one handles
// the event).
void com_send_event(event_t event);

void com_add_event_handler(event_handler_fn_t *handler, void *context, int priority);
void com_remove_event_handler(event_handler_fn_t *handler, void *context);
void com_clear_event_handlers(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW_EVENT_H__ include guard */
