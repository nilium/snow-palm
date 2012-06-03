/*
	Base entity component
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef COMPONENT_H

#define COMPONENT_H

#include "config.h"
#include "object.h"
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/*! Component object type. */
typedef struct s_component component_t;
/*! Component class type. */
typedef struct s_component_class component_class_t;
/*! Message type. */
typedef struct s_message message_t;
/*! Component function type. */
typedef void (*component_fn_t)(component_t *self);
/*! Component message handler function type. */
typedef bool (*component_msg_fn_t)(component_t *self, message_t message);

/*! Component flags. */
typedef enum
{
	/*! Marks a disabled. */
	COMP_DISABLED			= 1 << 1,
	/*! Marks a component as prepared (component_prepare has been called). */
	COMP_PREPARED			= 1 << 2,
	/*! Marks a component as started (component_start has been called). */
	COMP_STARTED			= 1 << 3,

	/*! Pseudo-flag, marks the beginning for flags in inheriting classes. */
	COMP_BEGIN_CUSTOM		= 1 << 8,
} component_flags_t;

struct s_component
{
	/*! Same as the isa member of an object, just uses the component class type
		instead for ease of use.
	*/
	component_class_t *isa;
	/*! Internal flags for tracking component state. */
	component_flags_t flags;
};

struct s_component_class
{
	class_t head;

	/*! The prepare method is called after all components in a given scene are
		loaded.  This allows it to initialize itself, begin grabbing other
		components, and otherwise do a whole bunch of stuff that's important.

		All components must implement this method to ensure calls to superclass
		implementations work.
	*/
	component_fn_t prepare;

	/*! The start method is called once before the first call to update, allowing
		the component to further prepare itself.  As such, this will not be called
		until the component is both enabled and going to be updated.

		All components must implement this method to ensure calls to superclass
		implementations work.
	*/
	component_fn_t start;

	/*! Updates the component - allows it to perform whatever it needs to on a per-
		per-frame basis.  This is only called while the component is enabled.

		All components must implement this method to ensure calls to superclass
		implementations work.
	*/
	component_fn_t update;

	/*! Receives a message directed either at the component, all components, or
		a specific kind of component.  This method should return true if it handled
		the message and false if it did not.
	*/
	component_msg_fn_t handle_message;
};

struct s_message
{
	/*! Sender of the message.  May be NULL. */
	object_t *sender;
	/*! The type of message - this can be any value. */
	int type;
	/*! pointer to some opaque data that the message may contain.  May be NULL. */
	void *opaque;
};

extern memory_pool_t gv_component_pool;
#define g_component_pool (&gv_component_pool)
extern const component_class_t g_component_class;
#define component_class (&g_component_class)

void sys_com_init(void);
void sys_com_shutdown(void);

/*! Instantiates a new component with the given class. */
component_t *com_new(component_class_t *cls);

/*! Makes the component prepare itself for use. */
void com_prepare(component_t *component);

/*! Informs the component that its update cycle will begin. */
void com_start(component_t *component);

/*! Sets whether a component is currently enabled. */
void com_set_enabled(component_t *component, bool enabled);

/*! Causes a component to run its update routine (this should be left untouched
	outside of the frame loop).
*/
void com_update(component_t *component);

/*** Messaging functions ***/
/*! Sends a message to a specific component.
	\returns True if the message was handled, false if not.
*/
bool com_msg(component_t *component, message_t message);
/*! Sends a message to a all components.
	\returns True if the message was handled by some component, false if not.
*/
bool com_msg_all(message_t message);
/*! Sends a message to a all components that are either an instance of the
	specified class or inherit from it.
	\returns True if the message was handled by some component, false if not.
*/
bool com_msg_all_of_kind(component_class_t *cls, message_t message);
/*! Sends a message to a all components of a specific class.
	\returns True if the message was handled by some component, false if not.
*/
bool com_msg_all_of_class(component_class_t *cls, message_t message);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: COMPONENT_H */

