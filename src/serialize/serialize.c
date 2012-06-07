#include "serialize.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


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


static inline sz_read_header(sz_context_t *ctx, void *p)
{

}


static inline buffer_t *
sz_new_compound(sz_context_t *ctx, void *p)
{
  uint32_t id = (uint32_t)array_size(&ctx->compounds) + 1;
  array_push(&ctx->compounds, NULL);
  buffer_t *buf = array_last(&ctx->compounds);
  buffer_init(buf, 32, ctx->alloc);
  map_insert(&ctx->compound_ptrs, p, (void *)id);
  return buf;
}


static inline void
sz_push_stack(sz_context_t *ctx)
{
  if (ctx->mode == SZ_READER) {
    fpos_t pos;
    fgetpos(ctx->file, &pos);
    array_push(&ctx->stack, &pos);
  } else {
    buffer_t *buffer = ctx->active;
    array_push(&ctx->stack, &buffer);
    ctx->active = array_last(&ctx->compounds);
  }
}

static inline void
sz_pop_stack(sz_context_t *ctx)
{
  if (ctx->mode == SZ_READER) {
    fpos_t pos;
    array_pop(&ctx->stack, &pos);
    fsetpos(ctx->file, &pos);
  } else {
    array_pop(&ctx->stack, &ctx->active);
  }
  return ctx->active;
}


sz_response_t
sz_free(sz_context_t *ctx, void *p)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  com_free(ctx->alloc, p);

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
  array_init(&ctx->stack, sizeof(fpos_t), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(fpos_t), 32, ctx->alloc);


  return SZ_SUCCESS;
}


static sz_response_t
sz_writer_begin(sz_context_t *ctx)
{
  array_init(&ctx->stack, sizeof(buffer_t *), 32, ctx->alloc);
  array_init(&ctx->compounds, sizeof(buffer_t), 32, ctx->alloc);
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
    .mappings_offset = 0,
    .compounds_offset = 0,
    .data_offset = (uint32_t)sizeof(root)
  };

  uint32_t relative_off;
  buffer_t *data = &ctx->buffer;
  size_t data_sz = buffer_size(data);
  void *data_ptr = buffer_pointer(data);

  fwrite(&root, sizeof(root), 1, file);

  relative_off = (uint32_t)(sizeof(root) + (root.num_compounds * sizeof(uint32_t)));
  for (index = 0, len = root.num_compounds; index < len; ++index) {
    fwrite(&relative_off, sizeof(uint32_t), 1, file);
  }

  for (index = 0; index < len; ++index) {
    buffer_t *comp_buf = array_at_index(&ctx->compounds, index);
    fwrite(buffer_pointer(comp_buf), buffer_size(comp_buf), 1, file);
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
  ctx->active = &ctx->buffer;

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
    return SZ_ERROR_INVALID_OPERATION;
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


static void
sz_write_compound_ref(sz_context_t *ctx, sz_compound_ref_t ref)
{

}


static uint32_t
sz_store_compound(sz_context_t *ctx, void *p,
                  sz_compound_writer_t writer, void *writer_ctx)
{
  uint32_t idx = (uint32_t)map_get(&ctx->compound_ptrs, p);
  if (idx != 0) return (idx - 1);

  idx = array_size(&ctx->compounds);
  sz_new_compound(ctx, p);
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

  ref.index = sz_store_compound(ctx, p, writer, ctx);

  buffer_write(ctx->active, &ref, sizeof(ref));

  return SZ_SUCCESS;
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
  return SZ_SUCCESS;
}


sz_response_t
sz_write_compounds(sz_context_t *ctx, uint32_t name, void **p, uint32_t length,
                       sz_compound_writer_t writer, void *writer_ctx)
{
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



  return SZ_SUCCESS;
}


sz_response_t
sz_read_compounds(sz_context_t *ctx, uint32_t name, void **p, uint32_t length,
                      sz_compound_reader_t writer, void *writer_ctx)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
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
sz_read_bytes(sz_context_t *ctx, uint32_t name, uint8_t **out, uint32_t *length)
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
  if (chunk.kind == SZ_BYTES_CHUNK && chunk.name == name) {
    size_t size;

    size = (size_t)chunk.size - sizeof(chunk);

    if (out) {
      uint8_t bytes = (uint8_t *)com_malloc(ctx->alloc, size);

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
  if (chunk.kind == SZ_UINT32_CHUNK && chunk.name == name) {
    fread(out, sizeof(float), 1, ctx->file);
  } else {
    fsetpos(ctx->file, &pos);
    return chunk.kind != SZ_FLOAT_CHUNK ? SZ_ERROR_WRONG_KIND : SZ_ERROR_BAD_NAME;
  }

  return SZ_SUCCESS;
}


sz_response_t
sz_read_floats(sz_context_t *ctx, uint32_t name, float **out, uint32_t *length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
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
sz_read_ints(sz_context_t *ctx, uint32_t name, int32_t **out, uint32_t *length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
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
sz_read_unsigned_ints(sz_context_t *ctx, uint32_t name, uint32_t **out, uint32_t *length)
{
  if (NULL == ctx) return SZ_ERROR_NULL_CONTEXT;

  if (ctx->mode == SZ_WRITER) {
    ctx->error = "Cannot perform read operation on write-serializer.";
    return SZ_ERROR_INVALID_OPERATION;
  }
  return SZ_SUCCESS;
}

#ifdef __cplusplus
}
#endif // __cplusplus
