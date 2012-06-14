#define S__NO_INLINE_STREAM_OPS
#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

stream_t *stream_alloc(stream_mode_t mode, allocator_t *alloc)
{
  stream_t *self;

  switch (mode) {
    case STREAM_READ:
    case STREAM_WRITE:
    case STREAM_READWRITE:
      break;
    default:
      s_log_error("Invalid stream mode %x.", mode);
      return NULL;
  }

  self = com_malloc(alloc, sizeof(*self));

  if (self) {
    self->alloc = alloc;

    self->mode = mode;

    self->read = NULL;
    self->write = NULL;
    self->seek = NULL;
    self->eof = NULL;
    self->close = NULL;
  } else {
    s_log_error("Failed to allocate stream.");
  }

  return self;
}


size_t stream_read(void * const ptr, size_t length, stream_t *stream)
{
  if (stream == NULL) {
    s_log_error("Stream is NULL");
    return 0;
  } else if (! (stream->mode & STREAM_READ)) {
    stream->error = STREAM_ERROR_READ_NOT_PERMITTED;
    return 0;
  } else if (stream->read == NULL) {
    stream->error = STREAM_ERROR_READ_NOT_SPECIFIED;
    return 0;
  } else if (ptr == NULL) {
    stream->error = STREAM_ERROR_NULL_POINTER;
    return 0;
  } else if (length == 0) {
    return 0;
  }

  return stream->read(ptr, length, stream);
}


size_t stream_write(const void * const ptr, size_t length, stream_t *stream)
{
  if (stream == NULL) {
    s_log_error("Stream is NULL");
    return 0;
  } else if (! (stream->mode & STREAM_WRITE)) {
    stream->error = STREAM_ERROR_WRITE_NOT_PERMITTED;
    return 0;
  } else if (stream->read == NULL) {
    stream->error = STREAM_ERROR_WRITE_NOT_SPECIFIED;
    return 0;
  } else if (ptr == NULL) {
    stream->error = STREAM_ERROR_NULL_POINTER;
    return 0;
  } else if (length == 0) {
    return 0;
  }

  return stream->write(ptr, length, stream);
}


off_t stream_seek(stream_t *stream, off_t off, int whence)
{
  if (stream == NULL) {
    s_log_error("Stream is NULL");
    return -1;
  }

  if (stream->seek == NULL) {
    stream->error = STREAM_ERROR_SEEK_NOT_SPECIFIED;
    return -1;
  }

  return stream->seek(stream, off, whence);
}


int stream_eof(stream_t *stream)
{
  if (!stream) {
    s_log_error("Stream is NULL");
    return -1;
  }

  if (! stream->eof) {
    stream->error = STREAM_ERROR_EOF_NOT_SPECIFIED;
    return -1;
  }

  return stream->eof(stream);
}


stream_error_t stream_close(stream_t *stream)
{
  stream_error_t error;

  if (!stream) {
    s_log_error("Stream is NULL");
    return STREAM_ERROR_NULL_STREAM;
  }

  error = STREAM_ERROR_NONE;

  // close the stream
  if (stream->close) {
    if (stream->close(stream)) {
      error = stream->error;

      if (error == STREAM_ERROR_CLOSE_NOT_PERMITTED)
        return error;
    }
  }
  // no error for when close isn't provided (might just be destroying the
  // stream immediately after stream_alloc for some reason)

  // free stream
  com_free(stream->alloc, stream);

  return error;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
