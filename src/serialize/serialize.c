#include "serialize.h"

#include <buffer/buffer_stream.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// There shouldn't be object trees deeper than this
#define SZ_MAX_STACK_SIZE (384)


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
static const char *sz_errstr_cannot_read = "Unable to read from file.";
static const char *sz_errstr_eof = "Unexpected end of file reached.";
static const char *sz_errstr_write_on_read = "Cannot perform write operation on read-serializer.";
static const char *sz_errstr_read_on_write = "Cannot perform read operation on write-serializer.";
static const char *sz_errstr_compound_reader_null = "Failed to deserialize compound object with reader: reader returned NULL.";
static const char *sz_errstr_already_closed = "Cannot close serializer that isn't open.";
static const char *sz_errstr_already_open = "Cannot set file for open serializer.";
static const char *sz_errstr_null_stream = "Stream is NULL.";
static const char *sz_errstr_empty_array = "Array is empty.";
static const char *sz_errstr_endian_open = "Cannot set endianness of open serializer.";


// static prototypes
static sz_response_t sz_file_error(sz_context_t *ctx);
static sz_response_t sz_read_root(sz_context_t *ctx, sz_root_t *root);
static sz_response_t sz_read_header(sz_context_t *ctx, sz_header_t *header, uint32_t name, uint8_t kind, bool null_allowed);
static sz_response_t sz_read_array_header(sz_context_t *ctx, sz_array_t *chunk, uint32_t name, uint8_t type);
static sz_response_t sz_read_array_body(sz_context_t *ctx, const sz_array_t *chunk, void **buf_out, size_t *length, allocator_t *alloc);
static uint32_t sz_new_compound(sz_context_t *ctx, void *p);
static void sz_push_stack(sz_context_t *ctx);
static void sz_pop_stack(sz_context_t *ctx);
static sz_response_t sz_write_null_pointer(sz_context_t *ctx, uint32_t name);
static sz_response_t sz_read_primitive(sz_context_t *ctx, uint8_t chunktype, uint32_t name, void *out, size_t typesize);
static sz_response_t sz_write_primitive(sz_context_t *ctx, uint8_t chunktype, uint32_t name, const void *input, size_t typesize);
static sz_response_t sz_write_primitive_array(sz_context_t *ctx, uint8_t type, uint32_t name, const void *values, size_t length, size_t element_size);
static sz_response_t sz_reader_begin(sz_context_t *ctx);
static sz_response_t sz_writer_begin(sz_context_t *ctx);
static sz_response_t sz_writer_flush(sz_context_t *ctx);
static uint32_t sz_store_compound(sz_context_t *ctx, void *p, sz_compound_writer_t writer, void *writer_ctx);

// inline statics
static inline uint32_t sz_to_endianness(uint32_t u, int src, int dst);
static inline sz_response_t sz_check_context(sz_context_t *ctx, sz_mode_t mode);


#define sz_read_with_bail(P, SIZE, STREAM, CTX) \
  if (stream_read((P), (SIZE), (STREAM)) != (SIZE))\
    return sz_file_error((CTX));


static sz_response_t
sz_file_error(sz_context_t *ctx)
{
  if (stream_eof(ctx->stream)) {
    ctx->error = sz_errstr_eof;
    return SZ_ERROR_EOF;
  } else {
    ctx->error = sz_errstr_cannot_read;
    return SZ_ERROR_CANNOT_READ;
  }
}


static sz_response_t
sz_read_root(sz_context_t *ctx, sz_root_t *root)
{
  sz_root_t res;

  sz_read_with_bail(&res, sizeof(sz_root_t), ctx->stream, ctx);

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
  uint32_t seq[2];
  sz_header_t res;

  sz_read_with_bail(&res.kind, sizeof(res.kind), ctx->stream, ctx);

  sz_read_with_bail(seq, sizeof(uint32_t) * 2, ctx->stream, ctx);

  res.name = seq[0];
  res.size = seq[1];

  if (header)
    *header = res;

  if ((header->kind == SZ_NULL_POINTER_CHUNK && !null_allowed) || header->kind != kind) {
    ctx->error = sz_errstr_wrong_kind;
    return SZ_ERROR_WRONG_KIND;
  } else if (header->name != name) {
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

  if (response != SZ_SUCCESS)
    return response;

  sz_read_with_bail(&res.length, sizeof(uint32_t), ctx->stream, ctx);

  sz_read_with_bail(&res.type, sizeof(uint8_t), ctx->stream, ctx);

  if (chunk)
    *chunk = res;

  return (res.type == type) ? SZ_SUCCESS : SZ_ERROR_WRONG_KIND;
}


static sz_response_t
sz_read_array_body(sz_context_t *ctx, const sz_array_t *chunk, void **buf_out, size_t *length, allocator_t *alloc) {
  size_t block_remainder;
  size_t element_size;
  void *buffer;

  if (chunk->header.kind == SZ_NULL_POINTER_CHUNK) {
    if (buf_out) *buf_out = NULL;
    if (length) *length = 0;
    return SZ_SUCCESS;
  }

  if (chunk->length == 0) {
    ctx->error = sz_errstr_empty_array;
    return SZ_ERROR_EMPTY_ARRAY;
  }

  if (alloc == NULL)
    alloc = g_default_allocator;

  block_remainder = (size_t)chunk->header.size - sizeof(*chunk);
  element_size = block_remainder / (size_t)chunk->length;

  if (buf_out) {
    size_t read_size = element_size * chunk->length;
    buffer = com_malloc(alloc, block_remainder);

    if (stream_read(buffer, read_size, ctx->stream) != read_size) {
      com_free(alloc, buffer);
      return sz_file_error(ctx);
    }

    if (buf_out)
      *buf_out = buffer;
  } else {
    stream_seek(ctx->stream, (off_t)block_remainder, SEEK_CUR);
  }

  if (length)
    *length = (size_t)chunk->length;

  return SZ_SUCCESS;
}


static inline uint32_t
sz_to_endianness(uint32_t u, int src, int dst)
{
  if (src == dst)
    return u;

  return ((u << 24) | (u & 0x0000FF00U << 8) | (u & 0x00FF0000U >> 8) | (u >> 24));
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
    .size = (uint32_t)(sizeof(chunk.kind) + sizeof(chunk.name) + sizeof(chunk.size))
  };

  stream_write(&chunk.kind, sizeof(chunk.kind), ctx->active);
  stream_write(&chunk.name, sizeof(chunk.name), ctx->active);
  stream_write(&chunk.size, sizeof(chunk.size), ctx->active);

  return SZ_SUCCESS;
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

  if (stream_read(out, typesize, ctx->stream) != typesize) {
    response = sz_file_error(ctx);
    goto sz_read_primitive_error;
  }

  return SZ_SUCCESS;

sz_read_primitive_error:
  stream_seek(ctx->stream, pos, SEEK_SET);
  return response;
}


static inline sz_response_t
sz_check_context(sz_context_t *ctx, sz_mode_t mode)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (mode != ctx->mode) {
    ctx->error = (mode == SZ_READER) ? sz_errstr_read_on_write :
                                       sz_errstr_write_on_read;
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
    .size = (uint32_t)(sizeof(chunk.kind) + sizeof(chunk.name) + sizeof(chunk.size) + typesize)
  };

  response = sz_check_context(ctx, SZ_WRITER);
  if (response != SZ_SUCCESS)
    return response;

  stream = ctx->active;

  stream_write(&chunk.kind, sizeof(chunk.kind), stream);
  stream_write(&chunk.name, sizeof(chunk.name), stream);
  stream_write(&chunk.size, sizeof(chunk.size), stream);
  stream_write(input, typesize, stream);

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
      .size = (uint32_t)(sizeof(chunk) + data_size)
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
  stream_write(&chunk.header.kind, sizeof(chunk.header.kind), stream);
  stream_write(&chunk.header.name, sizeof(chunk.header.name), stream);
  stream_write(&chunk.header.size, sizeof(chunk.header.size), stream);
  stream_write(&chunk.length, sizeof(chunk.length), stream);
  stream_write(&chunk.type, sizeof(chunk.type), stream);
  stream_write(values, data_size, stream);

  return SZ_SUCCESS;
}


sz_response_t
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
    ctx->endianness = SZ_LITTLE_ENDIAN;
    ctx->error = "";
    ctx->mode = mode;
    ctx->open = 0;
  }

  return ctx;
}


sz_response_t
sz_destroy_context(sz_context_t *ctx)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  memset(ctx, 0, sizeof(*ctx));

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

  array_init(&ctx->stack, sizeof(off_t), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(sz_unpacked_compound_t), 32, ctx->alloc);

  sz_push_stack(ctx);

  response = sz_read_root(ctx, &root);
  if (response != SZ_SUCCESS)
    return response;

  array_resize(&ctx->compounds, (size_t)root.num_compounds + 8);

  index = 0;
  len = root.num_compounds;
  packs = array_buffer(&ctx->compounds, NULL);

  if (packs == NULL && len != 0)
    s_fatal_error(1, "Failed to create unpacked compounds array");

  offsets_size = sizeof(uint32_t) * len;
  offsets = com_malloc(ctx->alloc, offsets_size);

  sz_read_with_bail(offsets, offsets_size, ctx->stream, ctx);

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
  ctx->buffer_stream = buffer_stream(&ctx->buffer, STREAM_WRITE, ctx->alloc);
  array_init(&ctx->stack, sizeof(stream_t *), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(sz_buffer_stream_t), 32, ctx->alloc);
  map_init(&ctx->compound_ptrs, g_mapops_default, ctx->alloc);

  ctx->active = ctx->buffer_stream;

  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_flush(sz_context_t *ctx)
{
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

  stream_write(&root, sizeof(root), stream);

  // write mappings
  relative_off = (uint32_t)(sizeof(root) + mappings_size);
  for (index = 0; index < len; ++index) {
    comp_buf = comp_buffers[index].buffer;
    stream_write(&relative_off, sizeof(uint32_t), stream);
    relative_off += buffer_size(comp_buf);
  }

  for (index = 0; index < len; ++index) {
    comp_buf = comp_buffers[index].buffer;
    stream_write(buffer_pointer(comp_buf), buffer_size(comp_buf), stream);
    stream_close(comp_buffers[index].stream);
  }

  if (data_ptr)
    stream_write(data_ptr, data_sz, stream);

  map_destroy(&ctx->compound_ptrs);

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

  if (ctx->mode == SZ_WRITER)
    res = sz_writer_flush(ctx);

  if (ctx->mode == SZ_WRITER)
    stream_close(ctx->buffer_stream);

  array_destroy(&ctx->compounds);
  array_destroy(&ctx->stack);

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


sz_response_t
sz_set_endianness(sz_context_t *ctx, sz_endianness_t endianness)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->open) {
    ctx->error = sz_errstr_endian_open;
    return SZ_ERROR_INVALID_OPERATION;
  }
  ctx->endianness = endianness;

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
    if (stream_read(&index, sizeof(uint32_t), ctx->stream) != sizeof(uint32_t)) {
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
  uint32_t index, ref;
  stream_t *stream;
  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(sizeof(chunk) + sizeof(uint32_t) * length)
    },
    .length = length,
    .type = SZ_COMPOUND_REF_CHUNK
  };

  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = sz_errstr_write_on_read;
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (p == NULL) {
    sz_write_null_pointer(ctx, name);
    return SZ_SUCCESS;
  }

  stream = ctx->active;
  stream_write(&chunk.header.kind, sizeof(chunk.header.kind), stream);
  stream_write(&chunk.header.name, sizeof(chunk.header.name), stream);
  stream_write(&chunk.header.size, sizeof(chunk.header.size), stream);
  stream_write(&chunk.length, sizeof(chunk.length), stream);
  stream_write(&chunk.type, sizeof(chunk.type), stream);

  for (index = 0; index < length; ++index) {
    ref = sz_store_compound(ctx, p[index], writer, writer_ctx);
    stream_write(&ref, sizeof(uint32_t), ctx->active);
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_compounds(sz_context_t *ctx, uint32_t name, void ***out, size_t *length,
                      sz_compound_reader_t reader, void *reader_ctx,
                      allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = sz_errstr_read_on_write;
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_response_t response;
  sz_array_t chunk;
  off_t pos;
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
        if (stream_read(&ref, sizeof(uint32_t), ctx->stream) != sizeof(uint32_t)) {
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
sz_write_bytes(sz_context_t *ctx, uint32_t name, const uint8_t *values, size_t length)
{
  return sz_write_primitive(ctx, SZ_BYTES_CHUNK, name, values, length * sizeof(uint8_t));
}


sz_response_t
sz_read_bytes(sz_context_t *ctx, uint32_t name, uint8_t **out, size_t *length, allocator_t *buf_alloc)
{
  sz_response_t response;
  sz_header_t chunk;
  off_t pos;
  size_t size;
  uint8_t *bytes;

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
    size = (size_t)chunk.size - sizeof(chunk);

    if (out) {
      bytes = (uint8_t *)com_malloc(buf_alloc, size);

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
