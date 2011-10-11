/*
	Object system / class
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef OBJECT_H

#define OBJECT_H

#include "config.h"
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_class class_t;
typedef struct s_object object_t;

typedef object_t *(*constructor_t)(object_t *self, memory_pool_t *pool);
typedef void (*destructor_t)(object_t *self);
typedef int (*comparator_t)(object_t *self, object_t *other);

struct s_class
{
	/* A class's superclass.  The superclass is the class that the
	   this class is inherited from (or the class that this clase is
	   based on, also called a "base class").
	*/
	const class_t *super;

	/* This is the size of an instance of this class, including the
	   size of the superclass's instance if one is present.
	*/
	size_t size;

	/* note on constructors:
		Constructors are not obligated to return the `self` object
		they receive - they can optionally autorelease it and return
		either NULL or a different object.  If it does return a
		different object, it should retain it before passing said
		object onto the next constructor.
	*/
	constructor_t ctor;
	/* Destructor function - this essentially finalizes the object
	   and prepares it to be freed - it never frees its own memory.

	   The destructor must never attempt to retain or otherwise cease
	   its own destruction.  If the destructor function is called
	   (by object_free), the object's memory _will_ be freed and all
	   pointers to that object will thus be invalid.
	*/
	destructor_t dtor;

	/* Comparison function - compares the self object to the other
	   object and returns 1 (one), 0 (zero), or -1 (negative one)
	   to indicate how the self stands in relation to other.  If
	   self is "greater than" other, return 1; if "less than" other,
	   return -1; and, if "equal", return 0.

	   This can be fishy due to objects of different types being
	   compared, and there not being a clear definition of when an
	   object is considered "greater than," "less than," or "equal"
	   to another object.  For example, two objects, despite being
	   different instances of a class (two different pointers), can
	   be equal due to their "content."
	*/
	comparator_t compare;
};

struct s_object
{
	const class_t *isa;
};

extern const class_t _object_class;
#define object_class (&_object_class)

/** The object system depends on the memory system. */
void sys_object_init(void);
void sys_object_shutdown(void);

/* Basic object comparison function.
   
   The default behavior for all objects is to compare the addresses of the
   objects to determine their order.  So, if the address of self is 1 and
   the address of other is 2, self is less than 1 and returns -1.
*/
int object_compare(object_t *self, object_t *other);

/* Basic object retain, release, autorelease, and retainCount functions.
   
   Unless required, one should not override these functions in subclasses
   of object_class (classes that inherit from object_class).
*/
object_t *object_retain(object_t *self);
object_t *object_autorelease(object_t *self);
void object_release(object_t *self);
int32_t object_retainCount(object_t *self);
/*! Determines whether the object is a kind of the class provided.

	\returns True the object inherits from this class or is an instance of the
	class, otherwise false.
*/
bool object_isKindOf(object_t *self, class_t *cls);
/*! Determines whether the object is an instance of the specific class provided.
	\returns True if the object is an instance of the class in question,
	otherwise false.  The object cannot be a descendant of the class, but must
	be an instance of that specific class.
*/
bool object_isClass(object_t *self, class_t *cls);

/* Object allocation and deletion functions.

   Due to the decision to use memory pools over standard allocation via malloc,
   you are required to pass either NULL or a valid memory pool to object_new.
   If the pool is NULL, the object will be allocated out of the global memory
   pool, as noted in mem_alloc (see memory.h).

   As a note, you should never explicitly call object_delete unless you're
   desperate to kill a particular object.  Leave it to the reference counting
   to determine when an object needs to be killed, that way it can be assumed
   that there are no strong references to the object in possession anymore.
*/
object_t *object_new(const class_t *cls, memory_pool_t *pool);
void object_delete(object_t *object);

/*! Stores a weak reference to value at the specified address.
	This will write value to the given address.  If NULL, it will also cease
	tracking weak references for the given address.
*/
void object_storeWeak(void *value, void **address);
/* Zeroes all weak references to the given value. */
void object_zeroWeak(void *value);

/*! Determines whether or not a class is also a kind of another class (i.e., if
	class inherited from the other class at some point). */
bool class_isKindOf(const class_t *cls, const class_t *other);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: OBJECT_H */
