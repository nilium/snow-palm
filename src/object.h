#ifndef OBJECT_H

#define OBJECT_H

#include "config.h"
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_class class_t;
typedef struct s_object *object_t;

typedef void *(*opaque_fn_t)();

struct s_class
{
	/* A class's superclass.  The superclass is the class that the
	   this class is inherited from (or the class that this clase is
	   based on, also called a "base class").
	*/
	const class_t *superClass;

	/* This is the size of an instance of this class, including the
	   size of the superclass's instance if one is present.
	*/
	size_t size;

	/*void *(*allocate)(size_t sz);*/
	/* note on constructors:
		Constructors are not obligated to return the `self` object
		they receive - they can optionally autorelease it and return
		either NULL or a different object.  If it does return a
		different object, it should retain it before passing said
		object onto the next constructor.
	*/
	object_t (*construct)(object_t self);

	/* Destructor function - this essentially finalizes the object
	   and prepares it to be freed - it never frees its own memory.

	   The destructor must never attempt to retain or otherwise cease
	   its own destruction.  If the destructor function is called
	   (by object_free), the object's memory _will_ be freed and all
	   pointers to that object will thus be invalid.
	*/
	void (*destruct)(object_t self);

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
	int (*compare)(object_t self, object_t other);

	/* Increments the retain count of `self.`  The caller of this
	   function indicates that it is taking ownership of the object.
	   
	   Every retain must be matched with a call to release (below).
	*/
	object_t (*retain)(object_t self);

	/* Decrements the retain count of `self.`  The caller of this
	   function is indicating that it is giving up ownership of the
	   object.

	   If an object's retain count becomes 0 (zero), the object is
	   then destroyed - its destructors are called and the object's
	   memory is freed.
	*/
	void (*release)(object_t self);

	/* This method places the object in the current thread's
	   auto-release pool.
	*/
	object_t (*autorelease)(object_t self);

	/* Returns the current retain count of the object `self`. */
	int32_t (*retainCount)(object_t self);

	/* Opaque group of function pointers.  Requires C99 support,
	   as this is a flexible array member.
	*/
	opaque_fn_t opaque[];
};

struct s_object
{
	const class_t *isa;
};

extern const class_t object_class;

void sys_object_init(void);
void sys_object_shutdown(void);

/* The basic object constructor. */
object_t object_ctor(object_t self);
/* The basic object destructor. */
void object_dtor(object_t self);

/* Basic object comparison function.
   
   The default behavior for all objects is to compare the addresses of the
   objects to determine their order.  So, if the address of self is 1 and
   the address of other is 2, self is less than 1 and returns -1.
*/
int object_compare(object_t self, object_t other);

/* Basic object retain, release, autorelease, and retainCount functions.
   
   Unless required, one should not override these functions in subclasses
   of object_class (classes that inherit from object_class).
*/
object_t object_retain(object_t self);
object_t object_autorelease(object_t self);
void object_release(object_t self);
int32_t object_retainCount(object_t self);

/* Object allocation and deletion functions.

   Due to the decision to use memory pools over standard allocation via malloc,
   you are required to pass either NULL or a valid memory pool to object_new.
   If the pool is NULL, the object will be allocated out of the global memory
   pool, as noted in mem_alloc (see memory.h).
*/
object_t object_new(class_t *cls, memory_pool_t *pool);
void object_delete(object_t object);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: OBJECT_H */
