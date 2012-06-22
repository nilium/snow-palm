#include "file_stream.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// STREAM OPS

static size_t file_write(const void * const p, size_t len, stream_t *buf);
static size_t file_read(void * const p, size_t len, stream_t *buf);
static off_t file_seek(stream_t *buf, off_t pos, int whence);
static int file_eof(stream_t *stream);
static int file_close(stream_t *stream);

// IMPLEMENTATION

stream_t *file_open(const char *path, stream_mode_t mode, allocator_t *alloc)
{
  PHYSFS_File *file = NULL;
  stream_t *stream = NULL;
  char *pathcopy;
  size_t pathsize;

  if (!alloc)
    alloc = g_default_allocator;

  if (!path) {
    s_log_error("NULL path for file.");
    return NULL;
  }

  stream = stream_alloc(mode, alloc);

  if (stream) {
    switch (mode) {
      case STREAM_READ:
        file = PHYSFS_openRead(path);
      break;
      case STREAM_WRITE:
        file = PHYSFS_openWrite(path);
      break;
      case STREAM_APPEND:
        file = PHYSFS_openAppend(path);
      break;
      default: /* cannot reach */ break;
    }

    // if r+ failed because the file doesn't exist, open again with w
    if (file == NULL) {
      s_log_error("Failed to open file '%s' with mode %d. Error: %s.",
        path, mode, pfs_get_error());
      stream_close(stream);
      return NULL;
    }

    stream->read = file_read;
    stream->write = file_write;
    stream->seek = file_seek;
    stream->eof = file_eof;
    stream->close = file_close;

    // copy path string
    pathsize = strlen(path) + 1;
    pathcopy = com_malloc(alloc, pathsize);
    strncpy(pathcopy, path, pathsize);

    stream->context.pfs.file = file;
    stream->context.pfs.file_path = pathcopy;
  }

  return stream;
}

static inline int file_check_context(stream_t *stream) {
  if (stream->context.pfs.file == NULL) {
    s_log_error("File backing stream is NULL.");
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return -1;
  } else if (stream->context.pfs.file_path == NULL) {
    s_log_error("File path for stream is NULL.");
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return -1;
  }

  return 0;
}

static size_t file_write(const void * const p, size_t len, stream_t *stream)
{
  PHYSFS_sint64 count;
  int error;
  PHYSFS_File *file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.pfs.file;

  count = PHYSFS_writeBytes(file, p, len);

  if (count < len && (error = PHYSFS_getLastErrorCode())) {
    s_log_error("Error writing to file stream (pfs: %s). (File: %s)",
      PHYSFS_getErrorByCode(error), stream->context.pfs.file_path);

    stream->error = STREAM_ERROR_FAILURE;

    if (count == -1)
      return 0;
  }

  return (size_t)count;
}

static size_t file_read(void * const p, size_t len, stream_t *stream)
{
  PHYSFS_sint64 count;
  int error;
  PHYSFS_File *file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.pfs.file;

  count = PHYSFS_readBytes(file, p, len);

  if (count < len && (error = PHYSFS_getLastErrorCode())) {
    s_log_error("Error reading from file stream (pfs: %s). (File: %s)",
      PHYSFS_getErrorByCode(error), stream->context.pfs.file_path);

    stream->error = STREAM_ERROR_FAILURE;

    if (count == -1)
      return 0;
  }

  return (size_t)count;
}

static off_t file_seek(stream_t *stream, off_t pos, int whence)
{
  PHYSFS_sint64 new_pos;
  PHYSFS_File *file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.pfs.file;
  new_pos = PHYSFS_tell(file);

  if (new_pos == -1) {
    s_log_error("Error getting file position: %s. (File: %s)",
      pfs_get_error(), stream->context.pfs.file_path);

    stream->error = STREAM_ERROR_FAILURE;

    return -1;
  }

  if (pos == 0 && whence == SEEK_CUR)
    return new_pos;

  switch (whence) {
    case SEEK_CUR:
      new_pos += pos;
      break;
    case SEEK_SET:
      new_pos = pos;
      break;
    case SEEK_END:
      new_pos = PHYSFS_fileLength(file) + pos;
      break;
  }

  if ( ! PHYSFS_seek(file, new_pos)) {
    s_log_error("Error seeking in file: %s. (File: %s)",
      pfs_get_error(), stream->context.pfs.file_path);

    stream->error = STREAM_ERROR_FAILURE;

    return -1;
  }

  return new_pos;
}

static int file_eof(stream_t *stream)
{
  PHYSFS_File *file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.pfs.file;

  return PHYSFS_eof(file);
}

static int file_close(stream_t *stream)
{
  PHYSFS_File *file;
  int r = 0;

  if (file_check_context(stream))
    r = -1;

  file = stream->context.pfs.file;

  if (file && ! PHYSFS_close(file)) {
    s_log_error("Error closing file: %s. (File: %s)",
      pfs_get_error(), stream->context.pfs.file_path);

    stream->error = STREAM_ERROR_FAILURE;
    r = -1;
  }

  if (stream->context.pfs.file_path)
    com_free(stream->alloc, stream->context.pfs.file_path);

  return r;
}


#ifdef __cplusplus
}
#endif // __cplusplus
