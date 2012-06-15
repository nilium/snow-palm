#include "file_stream.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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
  #define FILE_CHMOD (mode_t)(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
  int file_flags = 0;
  int file;
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
      case STREAM_READ: file_flags = O_RDONLY; break;
      case STREAM_WRITE: file_flags = O_WRONLY|O_TRUNC|O_CREAT; break;
      case STREAM_READWRITE: file_flags = O_RDWR|O_CREAT; break;
      default: /* cannot reach */ break;
    }

    file = open(path, file_flags, FILE_CHMOD);

    if (file < 0) {
      s_log_error("Failed to open file '%s' with flags %x. Error code %d.",
        path, file_flags, errno);
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
    stream->context.stdio.position = 0;
  }

  return stream;
}

static inline int file_check_context(stream_t *stream) {
  if (stream->context.stdio.file < 0) {
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
  ssize_t count;
  int error;
  int file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.stdio.file;

  count = write(file, p, len);

  if (count < 0 && (error = errno)) {
    s_log_error("Error writing to file stream (%d). (File: %s)",
      error, stream->context.stdio.file_path);
    stream->error = STREAM_ERROR_FAILURE;
  } else {
    stream->context.stdio.position += count;
  }

  return count;
}

static size_t file_read(void * const p, size_t len, stream_t *stream)
{
  ssize_t count;
  int error;
  int file;

  if (file_check_context(stream))
    return 0;

  file = stream->context.stdio.file;

  count = read(file, p, len);

  if (count < 0 && (error = errno)) {
    s_log_error("Error reading from file stream (%d). (File: %s)",
      error, stream->context.stdio.file_path);
    stream->error = STREAM_ERROR_FAILURE;
  } else {
    stream->context.stdio.position += count;
  }

  return count;
}

static off_t file_seek(stream_t *stream, off_t pos, int whence)
{
  off_t new_offset;
  int file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  if (pos == 0 && whence == SEEK_CUR)
    return stream->context.stdio.position;

  new_offset = lseek(file, pos, whence);

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

  stream->context.stdio.position = new_offset;

  return new_offset;
}

static int file_eof(stream_t *stream)
{
  int file;
  int test;
  ssize_t count;
  off_t pos;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  pos = stream->context.stdio.position;

  count = pread(file, &test, 1, pos);

  if (count == -1) {
    s_log_error("Error determining if file stream is at EOF: %d. (File: %s)",
      errno, stream->context.stdio.file_path);
    stream->error = STREAM_ERROR_FAILURE;
    return -1;
  }

  return count == 0;
}

static int file_close(stream_t *stream)
{
  int file;

  if (file_check_context(stream))
    return -1;

  file = stream->context.stdio.file;

  fsync(file);

  com_free(stream->alloc, stream->context.stdio.file_path);

  if (close(file)) {
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
