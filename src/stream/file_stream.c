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
  const char *file_mode = NULL;
  FILE *file = NULL;
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

    // TODO: rewrite to use PHYSFS

    switch (mode) {
      case STREAM_READ: file_mode = "r"; break;
      case STREAM_WRITE: file_mode = "w"; break;
      case STREAM_READWRITE: file_mode = "r+"; break;
      default: /* cannot reach */ break;
    }

    file = fopen(path, file_mode);

    // if r+ failed because the file doesn't exist, open again with w
    if (file == NULL && mode == STREAM_READWRITE && errno == ENOENT)
      file = fopen(path, (file_mode = "w+"));

    if (file == NULL) {
      s_log_error("Failed to open file '%s' with mode '%s'. Error code %d.",
        path, file_mode, errno);
      stream_close(stream);
      return NULL;
    }

    stream->read = file_read;
    stream->write = file_write;
    stream->seek = file_seek;
    stream->eof = file_eof;
    stream->close = file_close;

    // copy path string
    pathsize = sizeof(char) * (strlen(path) + 1);
    pathcopy = com_malloc(alloc, pathsize);
    strncpy(pathcopy, path, pathsize);

    stream->context.stdio.file = file;
    stream->context.stdio.file_path = pathcopy;
  }

  return stream;
}

static inline int file_check_context(stream_t *stream) {
  if (stream->context.stdio.file == NULL) {
    s_log_error("File backing stream is NULL.");
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return -1;
  } else if (stream->context.stdio.file_path == NULL) {
    s_log_error("File path for stream is NULL.");
    stream->error = STREAM_ERROR_INVALID_CONTEXT;
    return -1;
  }

  return 0;
}

static size_t file_write(const void * const p, size_t len, stream_t *stream)
{
  size_t count;
  int error;
  FILE *file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.stdio.file;

  count = fwrite(p, 1, len, file);

  if (count < len && (error = ferror(file))) {
    s_log_error("Error writing to file stream (%d). (File: %s)",
      error, stream->context.stdio.file_path);
    stream->error = STREAM_ERROR_FAILURE;
    clearerr(file);
  }

  return count;
}

static size_t file_read(void * const p, size_t len, stream_t *stream)
{
  size_t count;
  int error;
  FILE *file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.stdio.file;

  count = fread(p, 1, len, file);

  if (count < len && (error = ferror(file))) {
    s_log_error("Error reading from file stream (%d). (File: %s)",
      error, stream->context.stdio.file_path);
    stream->error = STREAM_ERROR_FAILURE;
    clearerr(file);
  }

  return count;
}

static off_t file_seek(stream_t *stream, off_t pos, int whence)
{
  off_t new_offset;
  FILE *file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  // doing nothing, use ftello
  if (pos == 0 && whence == SEEK_CUR)
    new_offset = ftello(file);
  else
    new_offset = fseeko(file, pos, whence);

  if (new_offset == -1) {
    switch (errno) {
      case EBADF:
        stream->error = STREAM_ERROR_SEEK_NOT_PERMITTED;
        s_log_error("File stream is not seekable. (File: %s)",
          stream->context.stdio.file_path);
      case EINVAL:
        stream->error = STREAM_ERROR_INVALID_WHENCE;
        s_log_error("Invalid whence argument to stream_seek. (File: %s)",
          stream->context.stdio.file_path);
        break;
      case EOVERFLOW:
        stream->error = STREAM_ERROR_OUT_OF_RANGE;
        s_log_error("Attempt to seek past the bounds of the file. (File: %s)",
          stream->context.stdio.file_path);
        break;
      case ESPIPE:
        stream->error = STREAM_ERROR_SEEK_NOT_PERMITTED;
        s_log_error("File stream is a pipe, cannot seek. (File: %s)",
          stream->context.stdio.file_path);
        break;
      default:
        stream->error = STREAM_ERROR_FAILURE;
        s_log_error("Undefined error while seeking in file stream: %d. (File: %s)",
          errno, stream->context.stdio.file_path);
        break;
    }
  }

  return new_offset;
}

static int file_eof(stream_t *stream)
{
  FILE *file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  return !!feof(file);
}

static int file_close(stream_t *stream)
{
  FILE *file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  com_free(stream->alloc, stream->context.stdio.file_path);

  if (fclose(file)) {
    int error = errno;
    stream->error = STREAM_ERROR_FAILURE;
    switch (error) {
      case EBADF:
        s_log_error("Error closing file, invalid file. (File: %s)",
          stream->context.stdio.file_path);
        break;
      case EINTR:
        s_log_error("Error closing file, signal interrupted closing. (File: %s)",
          stream->context.stdio.file_path);
        break;
      case EIO:
        s_log_error("Error closing file, IO error. (File: %s)",
          stream->context.stdio.file_path);
        break;
      default:
        s_log_error("Error closing file, unknown error code %d. (File: %s)",
          error, stream->context.stdio.file_path);
    }
    return -1;
  }

  return 0;
}


#ifdef __cplusplus
}
#endif // __cplusplus
