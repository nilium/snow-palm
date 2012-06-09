#include "serialize.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// There shouldn't be object trees deeper than this
#define SZ_MAX_STACK_SIZE (384)


typedef struct {
  // the position of the item in the file
  fpos_t position;
  // NULL if not yet unpacked
  void *value;
} sz_unpacked_compound_t;


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
static const char *sz_errstr_null_file = "File is NULL.";
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


static sz_response_t
sz_file_error(sz_context_t *ctx)
{
  if (feof(ctx->file)) {
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

  if (fread(&res, sizeof(sz_root_t), 1, ctx->file) != 1)
    return sz_file_error(ctx);

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

  if (fread(&res.kind, sizeof(res.kind), 1, ctx->file) != 1)
    return sz_file_error(ctx);

  if (fread(seq, sizeof(uint32_t), 2, ctx->file) != 2)
    return sz_file_error(ctx);

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

  if (fread(&res.length, sizeof(uint32_t), 1, ctx->file) != 1)
    return sz_file_error(ctx);

  if (fread(&res.type, sizeof(uint8_t), 1, ctx->file) != 1)
    return sz_file_error(ctx);

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
    buffer = com_malloc(alloc, block_remainder);

    if (fread(buffer, element_size, chunk->length, ctx->file) != chunk->length) {
      com_free(alloc, buffer);
      return sz_file_error(ctx);
    }

    if (buf_out)
      *buf_out = buffer;
  } else {
    fseeko(ctx->file, (off_t)block_remainder, SEEK_CUR);
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
  buffer_t *buffer = (buffer_t *)com_malloc(ctx->alloc, sizeof(*buffer));
  buffer_init(buffer, 32, ctx->alloc);
  array_push(&ctx->compounds, &buffer);
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
    array_push(&ctx->stack, NULL);
    fgetpos(ctx->file, array_last(&ctx->stack));
  } else {
    buffer_t *buffer = ctx->active;
    array_push(&ctx->stack, &buffer);
    ctx->active = *(buffer_t **)array_last(&ctx->compounds);
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
    fpos_t pos;
    array_pop(&ctx->stack, &pos);
    fsetpos(ctx->file, &pos);
  } else {
    buffer_t *step = NULL;
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

  buffer_write(ctx->active, &chunk.kind, sizeof(chunk.kind));
  buffer_write(ctx->active, &chunk.name, sizeof(chunk.name));
  buffer_write(ctx->active, &chunk.size, sizeof(chunk.size));

  return SZ_SUCCESS;
}


static sz_response_t
sz_read_primitive(sz_context_t *ctx,
                  uint8_t chunktype, uint32_t name,
                  void *out, size_t typesize)
{
  sz_response_t response;
  sz_header_t chunk;
  fpos_t pos;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  fgetpos(ctx->file, &pos);

  response = sz_read_header(ctx, &chunk, name, chunktype, false);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_error;

  if (fread(out, typesize, 1, ctx->file) != 1) {
    response = sz_file_error(ctx);
    goto sz_read_primitive_error;
  }

  return SZ_SUCCESS;

sz_read_primitive_error:
  fsetpos(ctx->file, &pos);
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
  buffer_t *buf;
  sz_header_t chunk = {
    .kind = chunktype,
    .name = name,
    .size = (uint32_t)(sizeof(chunk.kind) + sizeof(chunk.name) + sizeof(chunk.size) + typesize)
  };

  response = sz_check_context(ctx, SZ_WRITER);
  if (response != SZ_SUCCESS)
    return response;

  buf = ctx->active;

  buffer_write(buf, &chunk.kind, sizeof(chunk.kind));
  buffer_write(buf, &chunk.name, sizeof(chunk.name));
  buffer_write(buf, &chunk.size, sizeof(chunk.size));
  buffer_write(buf, input, typesize);

  return SZ_SUCCESS;
}


static sz_response_t
sz_write_primitive_array(sz_context_t *ctx,
                         uint8_t type, uint32_t name,
                         const void *values, size_t length, size_t element_size)
{
  sz_response_t response;
  buffer_t *buf;
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

  buf = ctx->active;

  // write each to deal with possible alignment/packing issues
  buffer_write(buf, &chunk.header.kind, sizeof(chunk.header.kind));
  buffer_write(buf, &chunk.header.name, sizeof(chunk.header.name));
  buffer_write(buf, &chunk.header.size, sizeof(chunk.header.size));
  buffer_write(buf, &chunk.length, sizeof(chunk.length));
  buffer_write(buf, &chunk.type, sizeof(chunk.type));
  buffer_write(buf, values, data_size);

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
  fpos_t pos;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS) return response;

  fgetpos(ctx->file, &pos);

  response = sz_read_array_header(ctx, &chunk, name, chunktype);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_array_error;

  response = sz_read_array_body(ctx, &chunk, out, length, buf_alloc);
  if (response != SZ_SUCCESS)
    goto sz_read_primitive_array_error;

  return SZ_SUCCESS;

sz_read_primitive_array_error:
  fsetpos(ctx->file, &pos);
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

    map_init(&ctx->compound_ptrs, g_mapops_default, alloc);
  }

  return ctx;
}


sz_response_t
sz_destroy_context(sz_context_t *ctx)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  array_destroy(&ctx->compounds);
  array_destroy(&ctx->stack);

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

  array_init(&ctx->stack, sizeof(fpos_t), 32, ctx->alloc);
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

  offsets = com_malloc(ctx->alloc, sizeof(uint32_t) * len);

  fread(offsets, sizeof(uint32_t), len, ctx->file);

  sz_pop_stack(ctx);

  for (; index < len; ++index) {
    off_t offset = (off_t)offsets[index];

    sz_push_stack(ctx);

    fseeko(ctx->file, offset, SEEK_CUR);
    fgetpos(ctx->file, &packs[index].position);

    sz_pop_stack(ctx);
  }

  com_free(ctx->alloc, offsets);

  fseeko(ctx->file, (off_t)root.data_offset, SEEK_CUR);

  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_begin(sz_context_t *ctx)
{
  buffer_init(&ctx->buffer, 32, ctx->alloc);
  array_init(&ctx->stack, sizeof(buffer_t *), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(buffer_t *), 32, ctx->alloc);

  ctx->active = &ctx->buffer;

  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_flush(sz_context_t *ctx)
{
  size_t index, len;

  FILE *file = ctx->file;

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
  buffer_t **comp_buffers = array_buffer(&ctx->compounds, NULL);
  buffer_t *comp_buf;

  uint32_t mappings_size = (uint32_t)sizeof(uint32_t) * root.num_compounds;
  uint32_t compounds_size = 0;

  for (index = 0, len = root.num_compounds; index < len; ++index)
    compounds_size += (uint32_t)buffer_size(comp_buffers[index]);

  root.compounds_offset = root.mappings_offset + mappings_size;
  root.data_offset = root.compounds_offset + compounds_size;
  root.size = root.data_offset + (uint32_t)buffer_size(&ctx->buffer);

  fwrite(&root, sizeof(root), 1, file);

  // write mappings
  relative_off = (uint32_t)(sizeof(root) + mappings_size);
  for (index = 0; index < len; ++index) {
    comp_buf = comp_buffers[index];
    fwrite(&relative_off, sizeof(uint32_t), 1, file);
    size_t sz = buffer_size(comp_buf);
    relative_off += sz;
  }

  for (index = 0; index < len; ++index) {
    comp_buf = comp_buffers[index];
    fwrite(buffer_pointer(comp_buf), buffer_size(comp_buf), 1, file);
    buffer_destroy(comp_buf);
  }

  if (data_ptr)
    fwrite(data_ptr, 1, data_sz, file);

  fflush(file);

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
    buffer_destroy(&ctx->buffer);

  ctx->open = 0;

  return res;
}


sz_response_t
sz_set_file(sz_context_t *ctx, FILE *file)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->open) {
    ctx->error = sz_errstr_already_open;
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (file == NULL) {
    ctx->error = sz_errstr_null_file;
    return SZ_ERROR_INVALID_FILE;
  }

  ctx->file = file;
  fgetpos(file, &ctx->file_pos);

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

    fsetpos(ctx->file, &pkg->position);

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
  fpos_t pos;
  uint32_t index;
  void *compound_ptr;
  sz_header_t chunk;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  fgetpos(ctx->file, &pos);

  response = sz_read_header(ctx, &chunk, name, SZ_COMPOUND_REF_CHUNK, true);
  if (response != SZ_SUCCESS)
    goto sz_read_compound_error;

  if (chunk.kind == SZ_COMPOUND_REF_CHUNK) {
    if (fread(&index, sizeof(uint32_t), 1, ctx->file) != 1) {
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
  fsetpos(ctx->file, &pos);
  return response;
}


sz_response_t
sz_write_compounds(sz_context_t *ctx, uint32_t name, void **p, size_t length,
                       sz_compound_writer_t writer, void *writer_ctx)
{
  uint32_t index, ref;
  buffer_t *buf;
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

  buf = ctx->active;
  buffer_write(buf, &chunk.header.kind, sizeof(chunk.header.kind));
  buffer_write(buf, &chunk.header.name, sizeof(chunk.header.name));
  buffer_write(buf, &chunk.header.size, sizeof(chunk.header.size));
  buffer_write(buf, &chunk.length, sizeof(chunk.length));
  buffer_write(buf, &chunk.type, sizeof(chunk.type));

  for (index = 0; index < length; ++index) {
    ref = sz_store_compound(ctx, p[index], writer, writer_ctx);
    buffer_write(ctx->active, &ref, sizeof(uint32_t));
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
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  response = sz_read_array_header(ctx, &chunk, name, SZ_COMPOUND_REF_CHUNK);
  if (response != SZ_SUCCESS) {
    fsetpos(ctx->file, &pos);
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
        fread(&ref, sizeof(uint32_t), 1, ctx->file);
        buf[index] = sz_get_compound(ctx, ref, reader, reader_ctx);
      }

      *out = buf;
    } else {
      fseek(ctx->file, sizeof(uint32_t) * chunk.length, SEEK_CUR);
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
  fpos_t pos;
  size_t size;
  uint8_t *bytes;

  response = sz_check_context(ctx, SZ_READER);
  if (response != SZ_SUCCESS)
    return response;

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  fgetpos(ctx->file, &pos);

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

      if (fread(bytes, 1, size, ctx->file) != size) {
        response = sz_file_error(ctx);
        goto sz_read_bytes_error;
      }

      *out = bytes;
    } else {
      fseek(ctx->file, size, SEEK_CUR);
    }

    if (length)
      *length = size;
  }

  return SZ_SUCCESS;

sz_read_bytes_error:
  fsetpos(ctx->file, &pos);
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
  if (out) *out = out_buf;
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
  if (out) *out = out_buf;
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
  if (out) *out = out_buf;
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
