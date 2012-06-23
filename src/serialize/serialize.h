#ifndef __SERIALIZE_H__
#define __SERIALIZE_H__ 1

#include <snow-config.h>
#include <buffer/buffer.h>
#include <structs/dynarray.h>
#include <structs/map.h>
#include <stream/stream.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SZ_MAGIC (0x31305A53)

// Chunk types
#define SZ_COMPOUND_CHUNK (1)
#define SZ_COMPOUND_REF_CHUNK (2)
#define SZ_FLOAT_CHUNK (3)
#define SZ_UINT32_CHUNK (4)
#define SZ_SINT32_CHUNK (5)
#define SZ_ARRAY_CHUNK (6)
#define SZ_BYTES_CHUNK (7)
// The null pointer chunk may substitute any compound, array, or bytes chunk
#define SZ_NULL_POINTER_CHUNK (8)
#define SZ_DOUBLE_CHUNK (9)

// Responses
typedef enum {
  SZ_SUCCESS = 0,
  SZ_ERROR_NONE = SZ_SUCCESS,
  // Root is invalid, typically meaning it's not actually a serializable stream.
  SZ_INVALID_ROOT,
  // Error when attempting to write an empty array (length == 0).
  SZ_ERROR_EMPTY_ARRAY,
  // A pointer is NULL.  For input operations, this means a required input
  // (e.g., an array of floats) is NULL.  For output operations, all pointers
  // other than the context are NULL when it's not permitted.  This may also
  // mean a required argument to a set_* function is NULL.
  SZ_ERROR_NULL_POINTER,
  // Exactly what it says on the tin.
  SZ_ERROR_NULL_CONTEXT,
  // The operation attempted cannot be performed.  This usually means
  // attempting to perform a read operation on a writable serializer.
  SZ_ERROR_INVALID_OPERATION,
  // The name being read is not the next name in the sequence
  SZ_ERROR_BAD_NAME,
  // What it says on the tin.  Again.
  SZ_ERROR_OUT_OF_MEMORY,
  // Attempted to read the wrong kind of data from the serializer.
  SZ_ERROR_WRONG_KIND,
  // Stream is somehow invalid
  SZ_ERROR_INVALID_STREAM,
  // Could not or cannot read from/write to the stream.
  SZ_ERROR_CANNOT_READ,
  SZ_ERROR_CANNOT_WRITE,
  // Could not or cannot write to a stream.
  // Reached EOF prematurely
  SZ_ERROR_EOF
} sz_response_t;

typedef enum {
  SZ_READER,
  SZ_WRITER
} sz_mode_t;

typedef struct s_sz_context sz_context_t;

typedef void (sz_compound_writer_t)(sz_context_t *ctx,
                                    void *p, void *writer_ctx);

/* An important note on how sz_compound_reader should work:
  When the reader is called, the output pointer, p, is only guaranteed to be
  valid before any other calls are made to the serializer. Second, the only way
  to deal with semi-circular references is to allocate, write p, and continue
  reading the contents of the compound only afterward.

  So, an example function might look like:

  sz_entity_reader(sz_context_t *ctx, void **p, void *rd_ctx)
  {
    char *name;
    entity_t *entity = entity_new(..);  // Allocate
    *p = entity;                        // Write to p
    // Do whatever else you need to, never access p again
    sz_read_bytes(ctx, KEY_ENTITY_NAME, &name);
    ...
  }
*/
typedef void (sz_compound_reader_t)(sz_context_t *ctx,
                                    void **p, void *reader_ctx);

struct s_sz_context {
  allocator_t *alloc;

  const char *error;

  int mode;
  int open;
  int compound_level;

  stream_t *stream;
  off_t stream_pos;

  stream_t *active;

  // writing: map of compounds in use to their indices
  map_t compound_ptrs;
  // stack that operates differently when reading and writing
  // writing: stack of active buffers
  // reading: stack of off_t locations in the stream
  array_t stack;
  // compound pointers
  // writing: pointers to buffers of compounds
  // reading: pointers to file offsets of compounds and their unpacked pointers
  array_t compounds;
  // output buffer
  // unused in reading
  buffer_t buffer;
  stream_t *buffer_stream;
};

typedef struct s_sz_root {
  uint32_t magic;
  // size of the serializable data including this root
  uint32_t size;
  // offsets are from the root
  uint32_t num_compounds;
  // Always immediately follows the root, so = sizeof(root)
  uint32_t mappings_offset;
  // Follows mappings
  uint32_t compounds_offset;
  // Follows compounds
  uint32_t data_offset;

  // mappings
  // data
  // compounds
} sz_root_t;

typedef struct s_sz_header {
  uint8_t kind;  // chunk type
  uint32_t name;  // chunk name
  uint32_t size;  // length including this header
} sz_header_t;

typedef struct s_sz_compound_ref {
  sz_header_t header;
  uint32_t index;
} sz_compound_ref_t;

typedef struct s_sz_array {
  sz_header_t header;
  uint32_t length;
  uint8_t type;
} sz_array_t;


sz_context_t *
sz_init_context(sz_context_t *ctx, sz_mode_t mode, allocator_t *alloc);
sz_response_t
sz_destroy_context(sz_context_t *ctx);

// Done with setting up, starts de/serialization.
sz_response_t
sz_open(sz_context_t *ctx);
// Ends de/serialization and closes the context.
// If serializing, the serialized data will be written to the file.
sz_response_t
sz_close(sz_context_t *ctx);

/////////// Attributes (use before sz_begin)

// Input / output file
sz_response_t
sz_set_stream(sz_context_t *ctx, stream_t *stream);

// Returns a NULL-terminated error string.
const char *
sz_get_error(sz_context_t *ctx);

//////////// Read/write operations

// Begins a compound for the pointer P
// Returns SZ_CONTINUE for
sz_response_t
sz_write_compound(sz_context_t *ctx, uint32_t name, void *p,
                  sz_compound_writer_t writer, void *writer_ctx);
sz_response_t
sz_read_compound(sz_context_t *ctx, uint32_t name, void **p,
                 sz_compound_reader_t reader, void *reader_ctx);

sz_response_t
sz_write_compounds(sz_context_t *ctx, uint32_t name,
                   void **out, size_t length,
                   sz_compound_writer_t writer, void *writer_ctx);
// Returns an array of pointers to the read compounds.
// The memory must be freed by the caller.
sz_response_t
sz_read_compounds(sz_context_t *ctx, uint32_t name,
                  void ***p, size_t *length,
                  sz_compound_reader_t reader, void *reader_ctx,
                  allocator_t *buf_alloc);

// Writes an array of bytes with the given length.
sz_response_t
sz_write_bytes(sz_context_t *ctx, uint32_t name,
               const void *values, size_t length);
// Reads an array of bytes and returns it via `out`.
// The memory must be freed by the caller.
sz_response_t
sz_read_bytes(sz_context_t *ctx, uint32_t name,
              void **out, size_t *length,
              allocator_t *buf_alloc);

sz_response_t
sz_write_float(sz_context_t *ctx, uint32_t name, float value);
sz_response_t
sz_write_floats(sz_context_t *ctx, uint32_t name,
                float *values, size_t length);

// Reads a single float and writes it to `out`.
sz_response_t
sz_read_float(sz_context_t *ctx, uint32_t name, float *out);
// Reads an array of floats and returns it via `out`.
// The memory must be freed by the caller.
sz_response_t
sz_read_floats(sz_context_t *ctx, uint32_t name,
               float **out, size_t *length,
               allocator_t *buf_alloc);

sz_response_t
sz_write_int(sz_context_t *ctx, uint32_t name, int32_t value);
sz_response_t
sz_write_ints(sz_context_t *ctx, uint32_t name,
              int32_t *values, size_t length);

// Reads a single int32_t and writes it to `out`.
sz_response_t
sz_read_int(sz_context_t *ctx, uint32_t name, int32_t *out);
// Reads an array of int32_t values and returns it via `out`.
// The memory must be freed by the caller.
sz_response_t
sz_read_ints(sz_context_t *ctx, uint32_t name,
             int32_t **out, size_t *length,
             allocator_t *buf_alloc);

sz_response_t
sz_write_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t value);
sz_response_t
sz_write_unsigned_ints(sz_context_t *ctx, uint32_t name,
                       uint32_t *values, size_t length);

// Reads a single uint32_t and writes it to `out`.
sz_response_t
sz_read_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t *out);
// Reads an array of uint32_t values and returns it via `out`.
// The memory must be freed by the caller.
sz_response_t
sz_read_unsigned_ints(sz_context_t *ctx, uint32_t name,
                      uint32_t **out, size_t *length,
                      allocator_t *buf_alloc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __sz_H__ include guard */
