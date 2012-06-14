#include "buffer_stream.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* context mapping:
  unknown[0] -> Buffer pointer
  unknown[1] -> Start pointer (to determine if the buffer has been resized)
  unknown[2] -> Offset pointer (read/write stream position)
  unknown[3] -> Destroy on close (casted)
*/

#define BUFFER_INDEX (0)
#define START_INDEX (1)
#define OFFSET_INDEX (2)
#define DESTROY_INDEX (3)

// STREAM OPS

static size_t buffer_write(const void * const p, size_t len, stream_t *stream);
static size_t buffer_read(void * const p, size_t len, stream_t *stream);
static off_t buffer_seek(stream_t *stream, off_t pos, int whence);
static int buffer_eof(stream_t *stream);
static int buffer_close(stream_t *stream);


// IMPLEMENTATION

stream_t *buffer_stream(buffer_t *buffer, stream_mode_t mode, bool destroy_on_close)
{
  if (!buffer)
    return NULL;

  stream_t *stream = stream_alloc(mode, buffer->alloc);

  if (stream) {
    stream->mode = mode;
    stream->read = buffer_read;
    stream->write = buffer_write;
    stream->seek = buffer_seek;
    stream->eof = buffer_eof;
    stream->close = buffer_close;
    stream->context.unknown[BUFFER_INDEX] = buffer;
    stream->context.unknown[START_INDEX] = buffer->ptr;
    stream->context.unknown[OFFSET_INDEX] = buffer->ptr;
    stream->context.unknown[DESTROY_INDEX] = (void *)destroy_on_close;
  }

  return stream;
}


// STREAM OPS

static int buffer_check_for_resize(stream_t *stream)
{
  buffer_t *buffer;
  char *context_ptr;
  char *buffer_ptr;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];

  if (! buffer) {
    s_log_error("Pointer to buffer in stream is NULL.");
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return 1;
  }

  context_ptr = (char *)stream->context.unknown[START_INDEX];
  buffer_ptr = buffer->ptr;

  if (context_ptr != buffer_ptr) {
    char *offset = (char *)stream->context.unknown[OFFSET_INDEX];

    ptrdiff_t diff = (offset - context_ptr);
    context_ptr = buffer->ptr;
    stream->context.unknown[START_INDEX] = context_ptr;
    stream->context.unknown[OFFSET_INDEX] = context_ptr + diff;
  }

  return 0;
}

static size_t buffer_write(const void * const p, size_t len, stream_t *stream)
{
  size_t abs_size;
  buffer_t *buffer;
  char *offset;

  if (buffer_check_for_resize(stream))
    return 0;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];
  offset = (char *)stream->context.unknown[OFFSET_INDEX];

  if (len == 0)
    return 0;

  abs_size = (size_t)((offset - buffer->ptr) + len);
  if (abs_size > buffer->size) {
    if (buffer->outside) {
      len -= abs_size - buffer->size;
    } else {
      if (buffer_resize(buffer, abs_size) == -1)
        return 0;

      if (buffer_check_for_resize(stream))
        return 0;

      offset = (char *)stream->context.unknown[OFFSET_INDEX];
    }
  }

  memcpy(offset, p, len);
  offset += len;

  stream->context.unknown[OFFSET_INDEX] = offset;

  return len;
}

static size_t buffer_read(void * const p, size_t len, stream_t *stream)
{
  buffer_t *buffer;
  char *offset;

  if (buffer_check_for_resize(stream))
    return 0;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];
  offset = (char *)stream->context.unknown[OFFSET_INDEX];

  if (offset + len >= buffer->ptr + buffer->size)
    len = ((buffer->ptr + buffer->size) - offset);

  memcpy(p, offset, len);
  offset += len;

  stream->context.unknown[OFFSET_INDEX] = offset;

  return len;
}

static off_t buffer_seek(stream_t *stream, off_t pos, int whence)
{
  buffer_t *buffer;
  char *new_offset;
  char *offset;
  ptrdiff_t abs_pos;

  if (buffer_check_for_resize(stream))
    return -1;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];
  offset = (char *)stream->context.unknown[OFFSET_INDEX];

  switch (whence) {
    case SEEK_SET:
      new_offset = buffer->ptr + pos;
      break;

    case SEEK_CUR:
      new_offset = offset + pos;
      break;

    case SEEK_END:
      new_offset = (buffer->ptr + buffer->size) + pos;
      break;

    default:
      stream->error = STREAM_ERROR_INVALID_WHENCE;
      return -1;
  }

  abs_pos = new_offset - buffer->ptr;
  // re: buffer->size + 1
  // Should be able to seek to at least the EOF point (which is start+size).
  if (buffer->ptr == NULL || abs_pos >= (buffer->size + 1) || abs_pos < 0) {
    stream->error = STREAM_ERROR_OUT_OF_RANGE;
    return -1;
  }

  stream->context.unknown[OFFSET_INDEX] = new_offset;

  return (off_t)abs_pos;
}

static int buffer_eof(stream_t *stream)
{
  buffer_t *buffer;
  char *offset;

  if (buffer_check_for_resize(stream))
    return -1;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];
  offset = (char *)stream->context.unknown[OFFSET_INDEX];

  return (offset == buffer->ptr + buffer->size);
}

static int buffer_close(stream_t *stream)
{
  bool destroy_on_close;
  buffer_t *buffer;

  buffer = (buffer_t *)stream->context.unknown[BUFFER_INDEX];
  if (! buffer) {
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return -1;
  }

  destroy_on_close = (bool)stream->context.unknown[DESTROY_INDEX];
  if (destroy_on_close) {
    if (buffer_destroy(buffer))
      return -1;
  }

  return 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus
