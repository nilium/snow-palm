#define __SNOW__SERIALIZE_C__

#include "serialize.h"

#include <buffer/buffer_stream.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// There shouldn't be object trees deeper than this
#define SZ_MAX_STACK_SIZE (384)
// The size of a chunk header
#define SZ_HEADER_SIZE (9)
// The size of an array chunk
#define SZ_ARRAY_SIZE (SZ_HEADER_SIZE + 5)


typedef struct {
  // the position of the item in the file
  off_t position;
  // NULL if not yet unpacked
  void *value;
} sz_unpacked_compound_t;

typedef struct {
  buffer_t *buffer;
  stream_t *stream;
} sz_buffer_stream_t;


static const char *sz_errstr_null_context = "Null serializer context.";
static const char *sz_errstr_invalid_root = "Invalid magic number for root.";
static const char *sz_errstr_wrong_kind = "Invalid chunk header: wrong chunk kind.";
static const char *sz_errstr_bad_name = "Invalid chunk header: wrong chunk name.";
static const char *sz_errstr_cannot_read = "Unable to read from stream.";
static const char *sz_errstr_cannot_write = "Unable to write to stream.";
static const char *sz_errstr_eof = "Unexpected end of stream reached.";
static const char *sz_errstr_write_on_read = "Cannot perform write operation on read-serializer.";
static const char *sz_errstr_read_on_write = "Cannot perform read operation on write-serializer.";
static const char *sz_errstr_compound_reader_null = "Failed to deserialize compound object with reader: reader returned NULL.";
static const char *sz_errstr_already_closed = "Cannot close serializer that isn't open.";
static const char *sz_errstr_already_open = "Cannot set stream for open serializer.";
static const char *sz_errstr_null_stream = "Stream is NULL.";
static const char *sz_errstr_empty_array = "Array is empty.";
static const char *sz_errstr_nomem = "Allocation failed.";


// static prototypes
// returns an error for the stream (never returns SZ_SUCCESS -- only call when you know an error has occurred)
static sz_response_t sz_file_error(sz_context_t *ctx);
// writes the file root
static sz_response_t sz_write_root(sz_context_t *ctx, sz_root_t root);
// reads the file root
static sz_response_t sz_read_root(sz_context_t *ctx, sz_root_t *root);
// reads a simple chunk header
static sz_response_t sz_read_header(sz_context_t *ctx, sz_header_t *header, uint32_t name, uint8_t kind, bool null_allowed);
// reads an array header
static sz_response_t sz_read_array_header(sz_context_t *ctx, sz_array_t *chunk, uint32_t name, uint8_t type);
// reads the body of a primitive array
static sz_response_t sz_read_array_body(sz_context_t *ctx, const sz_array_t *chunk, void **buf_out, size_t *length, allocator_t *alloc);
// allocates storage for a new compound
static uint32_t sz_new_compound(sz_context_t *ctx, void *p);
// stores a compound for writing later
static uint32_t sz_store_compound(sz_context_t *ctx, void *p, sz_compound_writer_t writer, void *writer_ctx);
// push/pop stack for reader/writer
static void sz_push_stack(sz_context_t *ctx);
static void sz_pop_stack(sz_context_t *ctx);
// writes a null pointer chunk
static sz_response_t sz_write_null_pointer(sz_context_t *ctx, uint32_t name);
// reads a single primitive
static sz_response_t sz_read_primitive(sz_context_t *ctx, uint8_t chunktype, uint32_t name, void *out, size_t typesize);
// writes a single primitive
static sz_response_t sz_write_primitive(sz_context_t *ctx, uint8_t chunktype, uint32_t name, const void *input, size_t typesize);
// writes an array of primitives
static sz_response_t sz_write_primitive_array(sz_context_t *ctx, uint8_t type, uint32_t name, const void *values, size_t length, size_t element_size);
// tells the reader to prepare itself
static sz_response_t sz_reader_begin(sz_context_t *ctx);
// tells the writer to prepare itself
static sz_response_t sz_writer_begin(sz_context_t *ctx);
// tells the writer to flush its buffers to the output stream
static sz_response_t sz_writer_flush(sz_context_t *ctx);
// releases memory for read/write contexts
static sz_response_t sz_destroy_reader(sz_context_t *ctx);
static sz_response_t sz_destroy_writer(sz_context_t *ctx);

// inline statics
static inline sz_response_t sz_write_header(sz_context_t *ctx, sz_header_t header);
static inline sz_response_t sz_check_context(sz_context_t *ctx, sz_mode_t mode);


static inline sz_response_t
sz_write_header(sz_context_t *ctx, sz_header_t header)
{
  stream_t *stream = ctx->active;

  if (   stream_write_uint8(stream, header.kind)
      || stream_write_uint32(stream, header.name)
      || stream_write_uint32(stream, header.size))
    return sz_file_error(ctx);

  return SZ_SUCCESS;
}


static sz_response_t
sz_file_error(sz_context_t *ctx)
{
  if (stream_eof(ctx->stream)) {
    ctx->error = sz_errstr_eof;
    return SZ_ERROR_EOF;
  } else {
    if (ctx->mode == SZ_READER) {
      ctx->error = sz_errstr_cannot_read;
      return SZ_ERROR_CANNOT_READ;
    } else { // writer
      ctx->error = sz_errstr_cannot_write;
      return SZ_ERROR_CANNOT_WRITE;
    }
  }
}


static sz_response_t
sz_write_root(sz_context_t *ctx, sz_root_t root)
{
  stream_t *stream = ctx->stream;
  if (   stream_write_uint32(stream, root.magic)
      || stream_write_uint32(stream, root.size)
      || stream_write_uint32(stream, root.num_compounds)
      || stream_write_uint32(stream, root.mappings_offset)
      || stream_write_uint32(stream, root.compounds_offset)
      || stream_write_uint32(stream, root.data_offset))
    return sz_file_error(ctx);

  return SZ_SUCCESS;
}


static sz_response_t
sz_read_root(sz_context_t *ctx, sz_root_t *root)
{
  sz_root_t res;
  stream_t *stream = ctx->stream;

  if (   stream_read_uint32(stream, &res.magic)
      || stream_read_uint32(stream, &res.size)
      || stream_read_uint32(stream, &res.num_compounds)
      || stream_read_uint32(stream, &res.mappings_offset)
      || stream_read_uint32(stream, &res.compounds_offset)
      || stream_read_uint32(stream, &res.data_offset))
    return sz_file_error(ctx);

  // TODO: if the serializer gets new versions, make sure to test for compat
  if (res.magic != SZ_MAGIC) {
    ctx->error = sz_errstr_invalid_root;
    return SZ_INVALID_ROOT;
  }

  *root = res;

  return SZ_SUCCESS;
}


static sz_response_t
sz_read_header(sz_context_t *ctx, sz_header_t *header, uint32_t name, uint8_t kind, bool null_allowed)
{
  sz_header_t res;
  stream_t *stream = ctx->stream;

  if (   stream_read_uint8(stream, &res.kind)
      || stream_read_uint32(stream, &res.name)
      || stream_read_uint32(stream, &res.size)) {

    return sz_file_error(ctx);

  }

  if (header)
    *header = res;

  if (  (res.kind == SZ_NULL_POINTER_CHUNK && !null_allowed)
      || res.kind != kind) {

    ctx->error = sz_errstr_wrong_kind;
    return SZ_ERROR_WRONG_KIND;

  } else if (res.name != name) {

    ctx->error = sz_errstr_bad_name;
    return SZ_ERROR_BAD_NAME;

  }


  return SZ_SUCCESS;
}


static sz_response_t
sz_read_array_header(sz_context_t *ctx, sz_array_t *chunk, uint32_t name, uint8_t type)
{
  sz_array_t res;
  sz_response_t response = sz_read_header(ctx, &res.header, name, SZ_ARRAY_CHUNK, true);
  stream_t *stream = ctx->stream;

  if (response != SZ_SUCCESS)
    return response;

  if (   stream_read_uint32(stream, &res.length)
      || stream_read_uint8(stream, &res.type))
    return sz_file_error(ctx);

  if (chunk)
    *chunk = res;

  return (res.type == type) ? SZ_SUCCESS : SZ_ERROR_WRONG_KIND;
}


static sz_response_t
sz_read_array_body(sz_context_t *ctx, const sz_array_t *chunk, void **buf_out, size_t *length, allocator_t *alloc) {
  off_t end_of_block;
  size_t block_remainder;
  size_t element_size;
  size_t arr_length;
  stream_t *stream;
  void *buffer;

  if (chunk->header.kind == SZ_NULL_POINTER_CHUNK) {
    if (buf_out) *buf_out = NULL;
    if (length) *length = 0;
    return SZ_SUCCESS;
  }

  arr_length = (size_t)chunk->length;

  if (arr_length == 0) {
    ctx->error = sz_errstr_empty_array;
    return SZ_ERROR_EMPTY_ARRAY;
  }

  if (alloc == NULL)
    alloc = g_default_allocator;

  stream = ctx->stream;
  block_remainder = (size_t)chunk->header.size - SZ_ARRAY_SIZE;
  element_size = block_remainder / arr_length;

  end_of_block = stream_tell(stream);
  if (end_of_block == -1)
    return sz_file_error(ctx);

  end_of_block += block_remainder;

  if (buf_out) {
    char *read_into;
    size_t index = 0;

    read_into = buffer = com_malloc(alloc, block_remainder);

    switch (element_size) {
      case 2:
        for (; index < arr_length; ++index, read_into += element_size)
          if (stream_read_uint16(stream, (uint16_t *)read_into))
            goto array_body_read_error;
      case 4:
        for (; index < arr_length; ++index, read_into += element_size)
          if (stream_read_uint32(stream, (uint32_t *)read_into))
            goto array_body_read_error;
      case 8:
        for (; index < arr_length; ++index, read_into += element_size)
          if (stream_read_uint64(stream, (uint64_t *)read_into))
            goto array_body_read_error;
      default:
        if (stream_read(buffer, block_remainder, ctx->stream) != block_remainder)
          goto array_body_read_error;
    }

    if (buf_out)
      *buf_out = buffer;
  } else {
    stream_seek(ctx->stream, (off_t)block_remainder, SEEK_CUR);
    ctx->error = sz_errstr_cannot_read;
    return SZ_ERROR_CANNOT_READ;
  }

  if (length)
    *length = (size_t)chunk->length;

  return SZ_SUCCESS;

array_body_read_error:
  stream_seek(ctx->stream, end_of_block, SEEK_SET);
  com_free(alloc, buffer);
  return sz_file_error(ctx);
}


static uint32_t
sz_new_compound(sz_context_t *ctx, void *p)
{
  uint32_t idx;
  sz_buffer_stream_t bs;
  buffer_t *buffer;

  buffer = com_malloc(ctx->alloc, sizeof(*buffer));
  buffer_init(buffer, 32, ctx->alloc);
  bs.buffer = buffer;
  bs.stream = buffer_stream(buffer, STREAM_WRITE, true);

  array_push(&ctx->compounds, &bs);

  idx = (uint32_t)array_size(&ctx->compounds);
  map_insert(&ctx->compound_ptrs, p, (void *)idx);

  return idx;
}


static void
sz_push_stack(sz_context_t *ctx)
{
  if (array_size(&ctx->stack) >= SZ_MAX_STACK_SIZE) {
    s_fatal_error(1, "Stack overflow in serializer");
    return;
  }

  if (ctx->mode == SZ_READER) {
    off_t pos = stream_tell(ctx->stream);
    array_push(&ctx->stack, &pos);
  } else {
    sz_buffer_stream_t *bs;
    stream_t *stream = ctx->active;

    array_push(&ctx->stack, &stream);

    bs = (sz_buffer_stream_t *)array_last(&ctx->compounds);
    ctx->active = bs->stream;
  }
}

static void
sz_pop_stack(sz_context_t *ctx)
{
  if (array_empty(&ctx->stack)) {
    s_fatal_error(1, "Stack underflow in serializer");
    return;
  }

  if (ctx->mode == SZ_READER) {
    off_t pos;
    array_pop(&ctx->stack, &pos);
    stream_seek(ctx->stream, pos, SEEK_SET);
  } else {
    stream_t *step = NULL;
    array_pop(&ctx->stack, &step);
    ctx->active = step;
  }
}


static sz_response_t
sz_write_null_pointer(sz_context_t *ctx, uint32_t name)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = sz_errstr_write_on_read;
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_header_t chunk = {
    .kind = SZ_NULL_POINTER_CHUNK,
    .name = name,
    .size = (uint32_t)(SZ_HEADER_SIZE)
  };

  return sz_write_header(ctx, chunk);
}


static sz_response_t
sz_read_primitive(sz_context_t *ctx,
                  uint8_t chunktype, uint32_t name,
                  void *out, size_t typesize)
{
  sz_response_t response;
  sz_header_t chunk;
  off_t pos;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  pos = stream_tell(ctx->stream);

  response = sz_read_header(ctx, &chunk, name, chunktype, false);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_error;

  switch(typesize) {
    case 2:
      if (stream_read_uint16(ctx->stream, out))
        goto sz_read_primitive_error;
      break;
    case 4:
      if (stream_read_uint32(ctx->stream, out))
        goto sz_read_primitive_error;
      break;
    case 8:
      if (stream_read_uint64(ctx->stream, out))
        goto sz_read_primitive_error;
      break;
    default:
      if (stream_read(out, typesize, ctx->stream) != typesize)
        goto sz_read_primitive_error;
  }

  return SZ_SUCCESS;

sz_read_primitive_error:
  response = sz_file_error(ctx);
  stream_seek(ctx->stream, pos, SEEK_SET);
  return response;
}


static inline sz_response_t
sz_check_context(sz_context_t *ctx, sz_mode_t mode)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (mode != ctx->mode) {
    ctx->error = (mode == SZ_READER) ? sz_errstr_read_on_write
                                     : sz_errstr_write_on_read;
    return SZ_ERROR_INVALID_OPERATION;
  }

  return SZ_SUCCESS;
}


static sz_response_t
sz_write_primitive(sz_context_t *ctx,
                   uint8_t chunktype, uint32_t name,
                   const void *input, size_t typesize)
{
  sz_response_t response;
  stream_t *stream;
  sz_header_t chunk = {
    .kind = chunktype,
    .name = name,
    .size = (uint32_t)(SZ_HEADER_SIZE + typesize)
  };

  response = sz_check_context(ctx, SZ_WRITER);
  if (response != SZ_SUCCESS)
    return response;

  stream = ctx->active;

  response = sz_write_header(ctx, chunk);
  if (response != SZ_SUCCESS)
    return response;

  switch (typesize) {
    case 2:
      if (stream_write_uint16(stream, *(const uint16_t *)input))
        return sz_file_error(ctx);
      break;
    case 4:
      if (stream_write_uint32(stream, *(const uint32_t *)input))
        return sz_file_error(ctx);
      break;
    case 8:
      if (stream_write_uint64(stream, *(const uint64_t *)input))
        return sz_file_error(ctx);
      break;
    default:
      if (stream_write(input, typesize, stream) != typesize)
        return sz_file_error(ctx);
  }

  return SZ_SUCCESS;
}


static sz_response_t
sz_write_primitive_array(sz_context_t *ctx,
                         uint8_t type, uint32_t name,
                         const void *values, size_t length, size_t element_size)
{
  sz_response_t response;
  stream_t *stream;
  size_t data_size = length * element_size;
  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(SZ_ARRAY_SIZE + data_size)
    },
    .length = (uint32_t)length,
    .type = type
  };

  response = sz_check_context(ctx, SZ_WRITER);
  if (response != SZ_SUCCESS)
    return response;

  if (values == NULL)
    return sz_write_null_pointer(ctx, name);

  stream = ctx->active;

  // write each to deal with possible alignment/packing issues
  response = sz_write_header(ctx, chunk.header);
  if (response != SZ_SUCCESS)
    return response;

  stream_write_uint32(stream, chunk.length);
  stream_write_uint8(stream, chunk.type);

  size_t idx = 0;
  switch (element_size) {
      case 2:
        for (; idx < length; ++idx)
          if (stream_write_uint16(stream, ((const uint16_t *)values)[idx]))
            return sz_file_error(ctx);
        break;
      case 4:
        for (; idx < length; ++idx)
          if (stream_write_uint32(stream, ((const uint32_t *)values)[idx]))
            return sz_file_error(ctx);
        break;
      case 8:
        for (; idx < length; ++idx)
          if (stream_write_uint64(stream, ((const uint64_t *)values)[idx]))
            return sz_file_error(ctx);
        break;
    default:
      if (stream_write(values, data_size, stream) != data_size)
        return sz_file_error(ctx);
  }

  return SZ_SUCCESS;
}


static sz_response_t
sz_read_primitive_array(sz_context_t *ctx,
                        uint8_t chunktype, uint32_t name,
                        void **out, size_t *length,
                        allocator_t *buf_alloc)
{
  sz_response_t response;
  sz_array_t chunk;
  off_t pos;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS) return response;

  pos = stream_tell(ctx->stream);

  response = sz_read_array_header(ctx, &chunk, name, chunktype);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_array_error;

  response = sz_read_array_body(ctx, &chunk, out, length, buf_alloc);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_array_error;

  return SZ_SUCCESS;

sz_read_primitive_array_error:
  stream_seek(ctx->stream, pos, SEEK_SET);
  return response;
}


sz_context_t *
sz_init_context(sz_context_t *ctx, sz_mode_t mode, allocator_t *alloc)
{
  if (ctx) {
    if (!alloc)
      alloc = g_default_allocator;

    memset(ctx, 0, sizeof(*ctx));

    ctx->alloc = alloc;
    ctx->error = "";
    ctx->mode = mode;
    ctx->open = 0;
  }

  return ctx;
}


sz_response_t
sz_destroy_context(sz_context_t *ctx)
{
  sz_response_t res = SZ_SUCCESS;

  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->open) {
    switch (ctx->mode) {
      case SZ_READER: res = sz_destroy_reader(ctx); break;
      case SZ_WRITER: res = sz_destroy_writer(ctx); break;
    }
  }

  memset(ctx, 0, sizeof(*ctx));

  return res;
}


static sz_response_t sz_destroy_reader(sz_context_t *ctx)
{
  array_destroy(&ctx->compounds);
  array_destroy(&ctx->stack);

  return SZ_SUCCESS;
}


static sz_response_t sz_destroy_writer(sz_context_t *ctx)
{
  size_t index, len;
  allocator_t *alloc = ctx->alloc;
  sz_buffer_stream_t *buffers = array_buffer(&ctx->compounds, NULL);

  map_destroy(&ctx->compound_ptrs);
  array_destroy(&ctx->compounds);
  array_destroy(&ctx->stack);
  stream_close(ctx->buffer_stream);

  len = array_size(&ctx->compounds);

  for (index = 0; index < len; ++index) {
    stream_close(buffers[index].stream);
    com_free(alloc, buffers[index].buffer);
  }

  return SZ_SUCCESS;
}


static sz_response_t
sz_reader_begin(sz_context_t *ctx)
{
  sz_root_t root;
  sz_response_t response;
  size_t index;
  size_t len;
  // arrays
  sz_unpacked_compound_t *packs;
  uint32_t *offsets;
  size_t offsets_size;
  stream_t *stream = ctx->stream;

  array_init(&ctx->stack, sizeof(off_t), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(sz_unpacked_compound_t), 32, ctx->alloc);

  sz_push_stack(ctx);

  response = sz_read_root(ctx, &root);
  if (response != SZ_SUCCESS)
    return response;

  array_resize(&ctx->compounds, (size_t)root.num_compounds + 8);

  len = root.num_compounds;
  packs = array_buffer(&ctx->compounds, NULL);

  if (packs == NULL && len != 0) {
    s_fatal_error(1, "Failed to create unpacked compounds array");
    ctx->error = sz_errstr_nomem;
    return SZ_ERROR_NULL_POINTER;
  }

  offsets_size = sizeof(uint32_t) * len;
  offsets = com_malloc(ctx->alloc, offsets_size);

  for (index = 0; index < len; ++index) {
    if (stream_read_uint32(stream, offsets + index)) {
      sz_pop_stack(ctx);
      return sz_file_error(ctx);
    }
  }

  sz_pop_stack(ctx);

  for (; index < len; ++index) {
    off_t offset = (off_t)offsets[index];

    sz_push_stack(ctx);

    stream_seek(ctx->stream, offset, SEEK_CUR);
    packs[index].position = ctx->stream_pos + stream_tell(ctx->stream);

    sz_pop_stack(ctx);
  }

  com_free(ctx->alloc, offsets);

  stream_seek(ctx->stream, (off_t)root.data_offset, SEEK_CUR);

  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_begin(sz_context_t *ctx)
{
  buffer_init(&ctx->buffer, 32, ctx->alloc);
  ctx->buffer_stream = buffer_stream(&ctx->buffer, STREAM_WRITE, true);
  array_init(&ctx->stack, sizeof(stream_t *), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(sz_buffer_stream_t), 32, ctx->alloc);
  map_init(&ctx->compound_ptrs, g_mapops_default, ctx->alloc);

  ctx->active = ctx->buffer_stream;

  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_flush(sz_context_t *ctx)
{
  sz_response_t response;
  size_t index, len;

  stream_t *stream = ctx->stream;

  sz_root_t root = {
    .magic = SZ_MAGIC,
    .size = 0,
    .num_compounds = array_size(&ctx->compounds),
    .mappings_offset = sizeof(root),
    .compounds_offset = (uint32_t)-1,
    .data_offset = (uint32_t)-1
  };

  uint32_t relative_off;
  buffer_t *data = &ctx->buffer;
  size_t data_sz = buffer_size(data);
  void *data_ptr = buffer_pointer(data);
  sz_buffer_stream_t *comp_buffers = array_buffer(&ctx->compounds, NULL);
  buffer_t *comp_buf;

  uint32_t mappings_size = (uint32_t)sizeof(uint32_t) * root.num_compounds;
  uint32_t compounds_size = 0;

  for (index = 0, len = root.num_compounds; index < len; ++index)
    compounds_size += (uint32_t)buffer_size(comp_buffers[index].buffer);

  root.compounds_offset = root.mappings_offset + mappings_size;
  root.data_offset = root.compounds_offset + compounds_size;
  root.size = root.data_offset + (uint32_t)buffer_size(&ctx->buffer);

  response = sz_write_root(ctx, root);
  if (response != SZ_SUCCESS)
    return response;

  // write mappings
  relative_off = (uint32_t)(sizeof(root) + mappings_size);
  for (index = 0; index < len; ++index) {
    comp_buf = comp_buffers[index].buffer;
    if (stream_write_uint32(stream, relative_off))
      return sz_file_error(ctx);
    relative_off += buffer_size(comp_buf);
  }

  for (index = 0; index < len; ++index) {
    size_t buffer_sz = buffer_size(comp_buf);
    comp_buf = comp_buffers[index].buffer;
    if (stream_write(buffer_pointer(comp_buf), buffer_sz, stream) != buffer_sz)
      return sz_file_error(ctx);
  }

  if (data_ptr && stream_write(data_ptr, data_sz, stream) != data_sz)
    return sz_file_error(ctx);

  return SZ_SUCCESS;
}


sz_response_t
sz_open(sz_context_t *ctx)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  ctx->open = 1;

  if (ctx->mode == SZ_READER)
    return sz_reader_begin(ctx);
  else
    return sz_writer_begin(ctx);
}


sz_response_t
sz_close(sz_context_t *ctx)
{
  sz_response_t res;

  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (1 != ctx->open) {
    ctx->error = sz_errstr_already_closed;
    return SZ_ERROR_INVALID_OPERATION;
  }

  res = SZ_SUCCESS;

  if (ctx->mode == SZ_WRITER) {
    res = sz_writer_flush(ctx);

    if (res != SZ_SUCCESS)
      return res;
  }

  switch (ctx->mode) {
    case SZ_READER: res = sz_destroy_reader(ctx); break;
    case SZ_WRITER: res = sz_destroy_writer(ctx); break;
  }

  ctx->open = 0;

  return res;
}


sz_response_t
sz_set_stream(sz_context_t *ctx, stream_t *stream)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->open) {
    ctx->error = sz_errstr_already_open;
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (NULL == stream) {
    ctx->error = sz_errstr_null_stream;
    return SZ_ERROR_INVALID_STREAM;
  }

  ctx->stream = stream;
  ctx->stream_pos = stream_tell(stream);

  return SZ_SUCCESS;
}


const char *
sz_get_error(sz_context_t *ctx)
{
  if (ctx == NULL)
    return sz_errstr_null_context;
  else
    return ctx->error;
}


static uint32_t
sz_store_compound(sz_context_t *ctx, void *p,
                  sz_compound_writer_t writer, void *writer_ctx)
{
  uint32_t idx;

  if (p == NULL)
    return 0;

  idx = (uint32_t)map_get(&ctx->compound_ptrs, p);
  if (idx != 0) return idx;

  idx = sz_new_compound(ctx, p);
  sz_push_stack(ctx);

  writer(ctx, p, writer_ctx);

  sz_pop_stack(ctx);

  return idx;
}


sz_response_t
sz_write_compound(sz_context_t *ctx, uint32_t name, void *p,
                      sz_compound_writer_t writer, void *writer_ctx)
{
  sz_response_t response;
  uint32_t index;

  response = sz_check_context(ctx, SZ_WRITER);
  if (response != SZ_SUCCESS)
    return response;

  if (p == NULL)
    return sz_write_null_pointer(ctx, name);

  index = sz_store_compound(ctx, p, writer, writer_ctx);

  return sz_write_primitive(ctx, SZ_COMPOUND_REF_CHUNK, name, &index, sizeof(uint32_t));
}


static void *
sz_get_compound(sz_context_t *ctx, uint32_t idx,
                sz_compound_reader_t reader, void *reader_ctx)
{
  sz_unpacked_compound_t *pkg;

  if (idx == 0)
    return NULL;

  pkg = array_at_index(&ctx->compounds, idx - 1);

  if (pkg->value == NULL) {
    sz_push_stack(ctx);

    stream_seek(ctx->stream, pkg->position, SEEK_SET);

    reader(ctx, &pkg->value, reader_ctx);

    sz_pop_stack(ctx);
  }

  return pkg->value;
}


sz_response_t
sz_read_compound(sz_context_t *ctx, uint32_t name, void **p,
                     sz_compound_reader_t reader, void *reader_ctx)
{
  sz_response_t response;
  off_t pos;
  uint32_t index;
  void *compound_ptr;
  sz_header_t chunk;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  pos = stream_tell(ctx->stream);

  response = sz_read_header(ctx, &chunk, name, SZ_COMPOUND_REF_CHUNK, true);
  if (response != SZ_SUCCESS)
    goto sz_read_compound_error;

  if (chunk.kind == SZ_COMPOUND_REF_CHUNK) {
    if (stream_read_uint32(ctx->stream, &index)) {
      response = sz_file_error(ctx);
      goto sz_read_compound_error;
    }

    compound_ptr = sz_get_compound(ctx, index, reader, reader_ctx);

    if (p)
      *p = compound_ptr;

    if (compound_ptr == NULL) {
      ctx->error = sz_errstr_compound_reader_null;
      response = SZ_ERROR_NULL_POINTER;
      goto sz_read_compound_error;
    }
  } else {
    // NULL chunk
    *p = NULL;
  }

  return SZ_SUCCESS;

sz_read_compound_error:
  stream_seek(ctx->stream, pos, SEEK_SET);
  return response;
}


sz_response_t
sz_write_compounds(sz_context_t *ctx, uint32_t name, void **p, size_t length,
                       sz_compound_writer_t writer, void *writer_ctx)
{
  sz_response_t response;
  uint32_t index, ref;
  stream_t *stream;
  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(SZ_ARRAY_SIZE + sizeof(uint32_t) * length)
    },
    .length = length,
    .type = SZ_COMPOUND_REF_CHUNK
  };

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  if (p == NULL)
    return sz_write_null_pointer(ctx, name);

  stream = ctx->active;

  response = sz_write_header(ctx, chunk.header);
  if (response != SZ_SUCCESS)
    return response;

  if (   stream_write_uint32(stream, chunk.length)
      || stream_write_uint8(stream, chunk.type))
    return sz_file_error(ctx);

  for (index = 0; index < length; ++index) {
    ref = sz_store_compound(ctx, p[index], writer, writer_ctx);
    stream_write_uint32(ctx->active, ref);
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_compounds(sz_context_t *ctx, uint32_t name, void ***out, size_t *length,
                      sz_compound_reader_t reader, void *reader_ctx,
                      allocator_t *buf_alloc)
{
  sz_response_t response;
  sz_array_t chunk;
  off_t pos;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  pos = stream_tell(ctx->stream);


  response = sz_read_array_header(ctx, &chunk, name, SZ_COMPOUND_REF_CHUNK);
  if (response != SZ_SUCCESS) {
    stream_seek(ctx->stream, pos, SEEK_SET);
    return response;
  }

  if (chunk.header.kind == SZ_NULL_POINTER_CHUNK && chunk.header.name == name) {
    *out = NULL;
    *length = 0;
  } else {
    if (out) {
      void **buf;
      uint32_t index;
      uint32_t ref;

      buf = (void **)com_malloc(buf_alloc, sizeof(void *) * chunk.length);

      for (index = 0; index < chunk.length; ++index) {
        if (stream_read_uint32(ctx->stream, &ref)) {
          com_free(buf_alloc, buf);
          stream_seek(ctx->stream, pos, SEEK_SET);
          return sz_file_error(ctx);
        }
        buf[index] = sz_get_compound(ctx, ref, reader, reader_ctx);
      }

      *out = buf;
    } else {
      stream_seek(ctx->stream, sizeof(uint32_t) * chunk.length, SEEK_CUR);
    }

    if (length)
      *length = (size_t)chunk.length;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_bytes(sz_context_t *ctx, uint32_t name, const void *values, size_t length)
{
  return sz_write_primitive(ctx, SZ_BYTES_CHUNK, name, values, length);
}


sz_response_t
sz_read_bytes(sz_context_t *ctx, uint32_t name, void **out, size_t *length, allocator_t *buf_alloc)
{
  sz_response_t response;
  sz_header_t chunk;
  off_t pos;
  size_t size;
  void *bytes;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  pos = stream_tell(ctx->stream);

  response = sz_read_header(ctx, &chunk, name, SZ_BYTES_CHUNK, true);
  if (response != SZ_SUCCESS)
    goto sz_read_bytes_error;

  if (chunk.kind == SZ_NULL_POINTER_CHUNK) {
    if (out) *out = NULL;
    if (length) *length = 0;
  } else {
    size = (size_t)chunk.size - SZ_HEADER_SIZE;

    if (out) {
      bytes = com_malloc(buf_alloc, size);

      if (stream_read(bytes, size, ctx->stream) != size) {
        response = sz_file_error(ctx);
        goto sz_read_bytes_error;
      }

      *out = bytes;
    } else {
      stream_seek(ctx->stream, size, SEEK_CUR);
    }

    if (length)
      *length = size;
  }

  return SZ_SUCCESS;

sz_read_bytes_error:
  stream_seek(ctx->stream, pos, SEEK_SET);
  return response;
}


sz_response_t
sz_write_float(sz_context_t *ctx, uint32_t name, float value)
{
  return sz_write_primitive(ctx, SZ_FLOAT_CHUNK, name, &value, sizeof(float));
}


sz_response_t
sz_write_floats(sz_context_t *ctx, uint32_t name, float *values, size_t length)
{
  return sz_write_primitive_array(ctx, SZ_FLOAT_CHUNK, name, values, length, sizeof(float));
}


sz_response_t
sz_read_float(sz_context_t *ctx, uint32_t name, float *out)
{
  float out_buf;
  sz_response_t ret = sz_read_primitive(ctx, SZ_FLOAT_CHUNK, name, &out_buf, sizeof(float));
  if (out && ret == SZ_SUCCESS) *out = out_buf;
  return ret;
}


sz_response_t
sz_read_floats(sz_context_t *ctx, uint32_t name, float **out, size_t *length, allocator_t *buf_alloc)
{
  return sz_read_primitive_array(ctx, SZ_FLOAT_CHUNK, name, (void **)out, length, buf_alloc);
}


sz_response_t
sz_write_int(sz_context_t *ctx, uint32_t name, int32_t value)
{
  return sz_write_primitive(ctx, SZ_SINT32_CHUNK, name, &value, sizeof(int32_t));
}


sz_response_t
sz_write_ints(sz_context_t *ctx, uint32_t name, int32_t *values, size_t length)
{
  return sz_write_primitive_array(ctx, SZ_SINT32_CHUNK, name, values, length, sizeof(int32_t));
}


sz_response_t
sz_read_int(sz_context_t *ctx, uint32_t name, int32_t *out)
{
  int32_t out_buf;
  sz_response_t ret = sz_read_primitive(ctx, SZ_SINT32_CHUNK, name, &out_buf, sizeof(int32_t));
  if (out && ret == SZ_SUCCESS) *out = out_buf;
  return ret;
}


sz_response_t
sz_read_ints(sz_context_t *ctx, uint32_t name, int32_t **out, size_t *length, allocator_t *buf_alloc)
{
  return sz_read_primitive_array(ctx, SZ_SINT32_CHUNK, name, (void **)out, length, buf_alloc);
}


sz_response_t
sz_write_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t value)
{
  return sz_write_primitive(ctx, SZ_UINT32_CHUNK, name, &value, sizeof(uint32_t));
}


sz_response_t
sz_write_unsigned_ints(sz_context_t *ctx, uint32_t name, uint32_t *values, size_t length)
{
  return sz_write_primitive_array(ctx, SZ_UINT32_CHUNK, name, values, length, sizeof(uint32_t));
}


sz_response_t
sz_read_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t *out)
{
  uint32_t out_buf;
  sz_response_t ret = sz_read_primitive(ctx, SZ_UINT32_CHUNK, name, &out_buf, sizeof(uint32_t));
  if (out && ret == SZ_SUCCESS) *out = out_buf;
  return ret;
}


sz_response_t
sz_read_unsigned_ints(sz_context_t *ctx, uint32_t name, uint32_t **out, size_t *length, allocator_t *buf_alloc)
{
  return sz_read_primitive_array(ctx, SZ_UINT32_CHUNK, name, (void **)out, length, buf_alloc);
}

#ifdef __cplusplus
}
#endif // __cplusplus
