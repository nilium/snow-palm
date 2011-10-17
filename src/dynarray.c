/*
	Dynamic array class
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "dynarray.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define ARRAY_ALLOC_TAG 0x00A77A70

static array_t *array_ctor(array_t *self, memory_pool_t *pool);
static void array_dtor(array_t *self);

const class_t g_array_class = {
	object_class,
	sizeof(array_t),

	(constructor_t)array_ctor,
	(destructor_t)array_dtor,

	object_compare
};

static array_t *array_ctor(array_t *self, memory_pool_t *pool)
{
	self = (array_t *)self->isa->super->ctor(self, pool);

	if (self) {
		self->size = 0;
		self->capacity = 0;
		self->buf = NULL;
		self->pool = mem_retain_pool(pool);
		self->obj_size = 0;
	}

	return self;
}

static void array_dtor(array_t *self)
{
	mem_free(self->buf);
	self->buf = NULL;
	self->size = self->capacity = self->obj_size = 0;
	mem_release_pool(self->pool);
	self->pool = NULL;

	self->isa->super->dtor(self);
}

array_t *array_init(array_t *self, size_t object_size, size_t size, size_t capacity)
{
	self->obj_size = object_size;
	
	array_reserve(self, capacity);
	array_resize(self, size);

	return self;
}

array_t *array_copy(array_t *other)
{
	array_t *self = array_init(object_new(array_class, other->pool),
		other->obj_size, other->size, other->capacity);

	if (self->size && self->buf)
		memcpy(self->buf, other->buf, self->size * self->obj_size);

	return self;
}

bool array_resize(array_t *self, size_t size)
{
	if (array_reserve(self, size)) {
		if (size < self->size && self->buf)
			memset(self->buf + (size * self->obj_size), 0, (self->capacity - self->size) * self->obj_size);
		self->size = size;

		return true;
	}

	return false;
}

bool array_reserve(array_t *self, size_t capacity)
{
	size_t new_size, new_cap, orig_size;
	bool tried_min = false;
	if (capacity <= self->capacity || capacity == 0) return true;

	new_cap = self->capacity * 2;
	if (new_cap < capacity) {
		new_cap = capacity; /* minimum requested is larger, use it */
		tried_min = true;
	}

	orig_size = self->size * self->obj_size;
reserve_capacity:
	new_size = new_cap * self->obj_size;

	char *new_buf = (char *)mem_alloc(self->pool, new_size, ARRAY_ALLOC_TAG);
	if (!new_buf) {
		/* in the event that the new buffer can't be allocated, try one more route
		   before giving up
		*/
		if (!tried_min) {
			/* if the minimum capacity requested hasn't been tried yet, try it */
			tried_min = true;
			new_cap = capacity;
			goto reserve_capacity;
		}

		log_error("Failed to reserve %zu elements for array.\n", new_cap);

		return false;
	}

	if (self->buf && orig_size)
		memcpy(new_buf, self->buf, orig_size);

	memset(new_buf + orig_size, 0, new_size - orig_size);
	if (self->buf) mem_free(self->buf);
	self->buf = new_buf;
	self->capacity = new_cap;

	return true;
}

size_t array_size(const array_t *self)
{
	return self->size;
}

size_t array_capacity(const array_t *self)
{
	return self->capacity;
}

void array_sort(array_t *self, int (*comparator)(const void *left, const void *right))
{
	if (self->size < 2)
		return;

	qsort(self->buf, self->size*self->obj_size, self->obj_size, comparator);
}

void *array_at_index(array_t *self, size_t index)
{
	if (self->size <= index)
		return NULL;
	
	return (self->buf + (index * self->obj_size));
}

bool array_push(array_t *self, const void *value)
{
	if (array_reserve(self, self->size + 1)) {
		memcpy(self->buf + (self->size * self->obj_size), value, self->obj_size);
		self->size += 1;
		return true;
	}

	return false;
}

void array_pop(array_t *self, void *result)
{
	char *addr = self->buf + ((self->size - 1) * self->obj_size);
	memcpy(result, addr, self->obj_size);
	memset(addr, 0, self->obj_size);
	self->size -= 1;
}

void *array_buffer(array_t *self, size_t *byte_length)
{
	if (byte_length)
		*byte_length = self->size * self->obj_size;
	return self->buf;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
