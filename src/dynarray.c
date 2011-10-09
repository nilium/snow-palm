#include "dynarray.h"

#define ARRAY_ALLOC_TAG 0x00A77A70

static array_t *array_ctor(array_t *self);
static void array_dtor(array_t *self);

const class_t array_class = {
	&object_class,
	sizeof(array_t),

	(constructor_t)array_ctor,
	(destructor_t)array_dtor,

	object_compare
};

static array_t *array_ctor(array_t *self)
{
	self->size = 0;
	self->capacity = 0;
	self->buf = NULL;
	self->pool = NULL;
	self->obj_size = 0;

	return self;
}

static void array_dtor(array_t *self)
{
	mem_free(self->buf);
	self->buf = NULL;
	self->size = self->capacity = self->obj_size = 0;
	mem_release_pool(self->pool);
	self->pool = NULL;
}

array_t *array_new(size_t objectSize, size_t size, size_t capacity, memory_pool_t *pool)
{
	array_t *self = (array_t *)object_new(&array_class, pool);

	self->pool = mem_retain_pool(pool);
	self->obj_size = objectSize;
	
	array_reserve(self, capacity);
	array_resize(self, size);

	return self;
}

array_t *array_copy(array_t *other)
{
	array_t *self = array_new(other->obj_size, other->size, other->capacity, other->pool);
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
	size_t newSize, newCap, origSize;
	bool triedMin = false;
	if (capacity <= self->capacity || capacity == 0) return true;

	newCap = self->capacity * 2;
	if (newCap < capacity) {
		newCap = capacity; /* minimum requested is larger, use it */
		triedMin = true;
	}

	origSize = self->size * self->obj_size;
reserve_capacity:
	newSize = newCap * self->obj_size;

	char *newBuf = (char *)mem_alloc(self->pool, newSize, ARRAY_ALLOC_TAG);
	if (!newBuf) {
		/* in the event that the new buffer can't be allocated, try one more route
		   before giving up
		*/
		if (!triedMin) {
			/* if the minimum capacity requested hasn't been tried yet, try it */
			triedMin = true;
			newCap = capacity;
			goto reserve_capacity;
		}

		log_error("Failed to reserve %zu elements for array.\n", newCap);

		return false;
	}

	if (self->buf && origSize)
		memcpy(newBuf, self->buf, origSize);

	memset(newBuf + origSize, 0, newSize - origSize);
	if (self->buf) mem_free(self->buf);
	self->buf = newBuf;
	self->capacity = newCap;

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

void *array_buffer(array_t *self, size_t *byteLength)
{
	if (byteLength)
		*byteLength = self->size * self->obj_size;
	return self->buf;
}

