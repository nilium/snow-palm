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


static inline uint32_t
sz_to_endianness(uint32_t u, int src, int dst) {
  if (src == dst)
    return u;

  return ((u << 24) | (u & 0x0000FF00U << 8) | (u & 0x00FF0000U >> 8) | (u >> 24));
}


static inline uint32_t
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


static inline void
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

static inline void
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
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_header_t chunk = {
    .kind = SZ_NULL_POINTER_CHUNK,
    .name = name,
    .size = (uint32_t)sizeof(chunk)
  };

  buffer_write(ctx->active, &chunk, sizeof(chunk));

  return SZ_SUCCESS;
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

  array_init(&ctx->stack, sizeof(fpos_t), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(sz_unpacked_compound_t), 0, ctx->alloc);

  sz_push_stack(ctx);

  fread(&root, sizeof(root), 1, ctx->file);
  if (root.magic != SZ_MAGIC) {
    ctx->error = "Invalid magic number for root.";
    return SZ_INVALID_ROOT;
  }

  array_resize(&ctx->compounds, (size_t)root.num_compounds + 8);

  size_t index = 0;
  size_t len = root.num_compounds;

  sz_unpacked_compound_t *packs = array_buffer(&ctx->compounds, NULL);
  if (packs == NULL && len != 0)
    s_fatal_error(1, "Failed to create unpacked compounds array");

  uint32_t *offsets = com_malloc(ctx->alloc, sizeof(uint32_t) * len);
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
    ctx->error = "Cannot end serializer that hasn't started.";
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
    ctx->error = "Cannot set file for active serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (file == NULL) {
    ctx->error = "File is NULL";
    return SZ_ERROR_INVALID_FILE;
  }

  ctx->file = file;
  fgetpos(file, &ctx->file_pos);

  return SZ_SUCCESS;
}


allocator_t *
sz_get_allocator(sz_context_t *ctx)
{
  if (NULL == ctx) return NULL;

  return ctx->alloc;
}


sz_response_t
sz_set_endianness(sz_context_t *ctx, sz_endianness_t endianness)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->open) {
    ctx->error = "Cannot set endianness of active serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }
  ctx->endianness = endianness;

  return SZ_SUCCESS;
}


const char *
sz_get_error(sz_context_t *ctx)
{
  if (ctx == NULL)
    return "Null serializer context.";
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
  sz_compound_ref_t ref = {
    .header = {
      .kind = SZ_COMPOUND_REF_CHUNK,
      .name = name,
      .size = (uint32_t)sizeof(ref)
    }
  };

  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (p == NULL)
    return sz_write_null_pointer(ctx, name);

  ref.index = sz_store_compound(ctx, p, writer, writer_ctx);

  buffer_write(ctx->active, &ref, sizeof(ref));

  return SZ_SUCCESS;
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
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  fpos_t pos;
  sz_header_t chunk;
  fgetpos(ctx->file, &pos);

  fread(&chunk, sizeof(chunk), 1, ctx->file);
  if (chunk.kind == SZ_NULL_POINTER_CHUNK && chunk.name == name) {
    *p = NULL;
  } else if (chunk.kind == SZ_COMPOUND_REF_CHUNK && chunk.name == name) {
    uint32_t index;
    void *ptr = NULL;
    fread(&index, sizeof(uint32_t), 1, ctx->file);

    ptr = sz_get_compound(ctx, index, reader, reader_ctx);

    if (p) *p = ptr;

    if (ptr == NULL) {
      ctx->error = "Failed to deserialize compound object with reader.";
      return SZ_ERROR_INVALID_OPERATION;
    }
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_FLOAT_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_compounds(sz_context_t *ctx, uint32_t name, void **p, uint32_t length,
                       sz_compound_writer_t writer, void *writer_ctx)
{
  uint32_t index, ref;

  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(sizeof(chunk) + sizeof(uint32_t) * length)
    },
    .type = SZ_COMPOUND_REF_CHUNK,
    .length = length
  };

  if (p == NULL) {
    sz_write_null_pointer(ctx, name);
    return SZ_SUCCESS;
  }

  buffer_write(ctx->active, &chunk, sizeof(chunk));

  for (index = 0; index < length; ++index) {
    ref = sz_store_compound(ctx, p[index], writer, writer_ctx);
    buffer_write(ctx->active, &ref, sizeof(uint32_t));
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_compounds(sz_context_t *ctx, uint32_t name, void ***out, uint32_t *length,
                      sz_compound_reader_t reader, void *reader_ctx,
                      allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_array_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk.header, sizeof(chunk.header), 1, ctx->file);
  if (chunk.header.kind == SZ_NULL_POINTER_CHUNK && chunk.header.name == name) {
    *out = NULL;
    *length = 0;
  } else if (chunk.header.kind == SZ_ARRAY_CHUNK && chunk.header.name == name) {
    fread(&chunk.type, sizeof(uint32_t), 2, ctx->file);

    if (chunk.type == SZ_COMPOUND_REF_CHUNK) {
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
        *length = chunk.length;

      return SZ_SUCCESS;
    } else {
      fsetpos(ctx->file, &pos);
      return SZ_ERROR_WRONG_KIND;
    }
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.header.kind != SZ_UINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_bytes(sz_context_t *ctx, uint32_t name, const uint8_t *values, uint32_t length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (values == NULL)
    return sz_write_null_pointer(ctx, name);

  sz_header_t chunk = {
    .kind = SZ_BYTES_CHUNK,
    .name = name,
    .size = (uint32_t)(sizeof(chunk) + length)
  };

  buffer_write(ctx->active, &chunk, sizeof(chunk));
  buffer_write(ctx->active, values, length);

  return SZ_SUCCESS;
}


sz_response_t
sz_read_bytes(sz_context_t *ctx, uint32_t name, uint8_t **out, uint32_t *length, allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_header_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk, sizeof(chunk), 1, ctx->file);
  if (chunk.kind == SZ_NULL_POINTER_CHUNK && chunk.name == name) {
    *out = NULL;
    *length = 0;
  } else if (chunk.kind == SZ_BYTES_CHUNK && chunk.name == name) {
    uint32_t size;

    size = chunk.size - (uint32_t)sizeof(chunk);

    if (out) {
      uint8_t *bytes = (uint8_t *)com_malloc(buf_alloc, size);

      fread(bytes, 1, size, ctx->file);

      *out = bytes;
    } else {
      fseek(ctx->file, size, SEEK_CUR);
    }

    if (length)
      *length = size;

  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_BYTES_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_float(sz_context_t *ctx, uint32_t name, float value)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_float_t chunk = {
    .header = {
      .kind = SZ_FLOAT_CHUNK,
      .name = name,
      .size = (uint32_t)sizeof(chunk)
    },
    .value = value
  };

  buffer_write(ctx->active, &chunk, sizeof(chunk));

  return SZ_SUCCESS;
}


sz_response_t
sz_write_floats(sz_context_t *ctx, uint32_t name, float *values, uint32_t length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (values == NULL)
    return sz_write_null_pointer(ctx, name);

  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(sizeof(chunk) + (length * sizeof(float)))
    },
    .type = SZ_FLOAT_CHUNK,
    .length = length
  };

  buffer_t *buf = ctx->active;
  buffer_write(buf, &chunk, sizeof(chunk));
  buffer_write(buf, values, sizeof(float) * length);

  return SZ_SUCCESS;
}


sz_response_t
sz_read_float(sz_context_t *ctx, uint32_t name, float *out)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_header_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk, sizeof(chunk), 1, ctx->file);
  if (chunk.kind == SZ_FLOAT_CHUNK && chunk.name == name) {
    fread(out, sizeof(float), 1, ctx->file);
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_FLOAT_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_floats(sz_context_t *ctx, uint32_t name, float **out, uint32_t *length, allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_array_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk.header, sizeof(chunk.header), 1, ctx->file);
  if (chunk.header.kind == SZ_NULL_POINTER_CHUNK && chunk.header.name == name) {
    *out = NULL;
    *length = 0;
  } else if (chunk.header.kind == SZ_ARRAY_CHUNK && chunk.header.name == name) {
    fread(&chunk.type, sizeof(uint32_t), 2, ctx->file);

    if (chunk.type == SZ_FLOAT_CHUNK) {
      if (out) {
        float *buf;

        buf = (float *)com_malloc(buf_alloc, sizeof(float) * chunk.length);

        fread(buf, sizeof(float), chunk.length, ctx->file);

        *out = buf;
      } else {
        fseek(ctx->file, sizeof(float) * chunk.length, SEEK_CUR);
      }

      if (length)
        *length = chunk.length;

      return SZ_SUCCESS;
    } else {
      fsetpos(ctx->file, &pos);
      return SZ_ERROR_WRONG_KIND;
    }
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.header.kind != SZ_UINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_int(sz_context_t *ctx, uint32_t name, int32_t value)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_int_t chunk = {
    .header = {
      .kind = SZ_SINT32_CHUNK,
      .name = name,
      .size = (uint32_t)sizeof(chunk)
    },
    .value = value
  };

  buffer_write(ctx->active, &chunk, sizeof(chunk));

  return SZ_SUCCESS;
}


sz_response_t
sz_write_ints(sz_context_t *ctx, uint32_t name, int32_t *values, uint32_t length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (values == NULL)
    return sz_write_null_pointer(ctx, name);

  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(sizeof(chunk) + (length * sizeof(uint32_t)))
    },
    .type = SZ_SINT32_CHUNK,
    .length = length
  };

  buffer_t *buf = ctx->active;
  buffer_write(buf, &chunk, sizeof(chunk));
  buffer_write(buf, values, sizeof(int32_t) * length);

  return SZ_SUCCESS;
}


sz_response_t
sz_read_int(sz_context_t *ctx, uint32_t name, int32_t *out)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_header_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk, sizeof(chunk), 1, ctx->file);
  if (chunk.kind == SZ_SINT32_CHUNK && chunk.name == name) {
    fread(out, sizeof(int32_t), 1, ctx->file);
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_SINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_ints(sz_context_t *ctx, uint32_t name, int32_t **out, uint32_t *length, allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_array_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk.header, sizeof(chunk.header), 1, ctx->file);
  if (chunk.header.kind == SZ_NULL_POINTER_CHUNK && chunk.header.name == name) {
    *out = NULL;
    *length = 0;
  } else if (chunk.header.kind == SZ_ARRAY_CHUNK && chunk.header.name == name) {
    fread(&chunk.type, sizeof(uint32_t), 2, ctx->file);

    if (chunk.type == SZ_SINT32_CHUNK) {
      if (out) {
        int32_t *buf;

        buf = (int32_t *)com_malloc(buf_alloc, sizeof(int32_t) * chunk.length);

        fread(buf, sizeof(int32_t), chunk.length, ctx->file);

        *out = buf;
      } else {
        fseek(ctx->file, sizeof(int32_t) * chunk.length, SEEK_CUR);
      }

      if (length)
        *length = chunk.length;

      return SZ_SUCCESS;
    } else {
      fsetpos(ctx->file, &pos);
      return SZ_ERROR_WRONG_KIND;
    }
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.header.kind != SZ_UINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_write_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t value)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_unsigned_int_t chunk = {
    .header = {
      .kind = SZ_UINT32_CHUNK,
      .name = name,
      .size = (uint32_t)sizeof(chunk)
    },
    .value = value
  };

  buffer_write(ctx->active, &chunk, sizeof(chunk));

  return SZ_SUCCESS;
}


sz_response_t
sz_write_unsigned_ints(sz_context_t *ctx, uint32_t name, uint32_t *values, uint32_t length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_READER) {
    ctx->error = "Cannot perform write operation on read-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (values == NULL)
    return sz_write_null_pointer(ctx, name);

  sz_array_t chunk = {
    .header = {
      .kind = SZ_ARRAY_CHUNK,
      .name = name,
      .size = (uint32_t)(sizeof(chunk) + (length * sizeof(uint32_t)))
    },
    .type = SZ_UINT32_CHUNK,
    .length = length
  };

  buffer_t *buf = ctx->active;
  buffer_write(buf, &chunk, sizeof(chunk));
  buffer_write(buf, values, sizeof(uint32_t) * length);

  return SZ_SUCCESS;
}


sz_response_t
sz_read_unsigned_int(sz_context_t *ctx, uint32_t name, uint32_t *out)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  sz_header_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk, sizeof(chunk), 1, ctx->file);
  if (chunk.kind == SZ_UINT32_CHUNK && chunk.name == name) {
    fread(out, sizeof(uint32_t), 1, ctx->file);
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_UINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_unsigned_ints(sz_context_t *ctx, uint32_t name, uint32_t **out, uint32_t *length, allocator_t *buf_alloc)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }

  if (buf_alloc == NULL)
    buf_alloc = g_default_allocator;

  sz_array_t chunk;
  fpos_t pos;
  fgetpos(ctx->file, &pos);

  fread(&chunk.header, sizeof(chunk.header), 1, ctx->file);
  if (chunk.header.kind == SZ_NULL_POINTER_CHUNK && chunk.header.name == name) {
    *out = NULL;
    *length = 0;
  } else if (chunk.header.kind == SZ_ARRAY_CHUNK && chunk.header.name == name) {
    fread(&chunk.type, sizeof(uint32_t), 2, ctx->file);

    if (chunk.type == SZ_UINT32_CHUNK) {
      if (out) {
        uint32_t *buf;

        buf = (uint32_t *)com_malloc(buf_alloc, sizeof(uint32_t) * chunk.length);

        fread(buf, sizeof(uint32_t), chunk.length, ctx->file);

        *out = buf;
      } else {
        fseek(ctx->file, sizeof(uint32_t) * chunk.length, SEEK_CUR);
      }

      if (length)
        *length = chunk.length;

      return SZ_SUCCESS;
    } else {
      fsetpos(ctx->file, &pos);
      return SZ_ERROR_WRONG_KIND;
    }
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.header.kind != SZ_UINT32_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}

#ifdef __cplusplus
}
#endif // __cplusplus
