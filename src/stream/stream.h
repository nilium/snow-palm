#ifndef __SNOW__STREAM_H__
#define __SNOW__STREAM_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>

#ifdef __SNOW__STREAM_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define STREAM_UNKNOWN_CONTEXT_COUNT (4)

typedef enum {
  // Opens the stream for reading only, starts at the beginning.
  STREAM_READ = 1,
  // Opens the stream for writing only, starts at the beginning.
  STREAM_WRITE = 2,
  // Opens the stream for writing, starts at the end of the file.
  STREAM_APPEND = 3
} stream_mode_t;

typedef enum {
  STREAM_ERROR_NONE = 0,

  // Only returned by stream_close when the stream is NULL.
  // Really should be checking for this in code using streams, but it needed an
  // error code. In debug mode, this is a fatal error. Don't let it happen.
  STREAM_ERROR_NULL_STREAM,

  // Invalid mode errors.
  STREAM_ERROR_READ_NOT_PERMITTED,
  STREAM_ERROR_WRITE_NOT_PERMITTED,
  // Errors that only the stream's functions can set.
  STREAM_ERROR_EOF_NOT_PERMITTED,
  STREAM_ERROR_SEEK_NOT_PERMITTED,
  // Think long and hard before using this.
  STREAM_ERROR_CLOSE_NOT_PERMITTED,

  // Errors for when a function for the stream wasn't provided.
  STREAM_ERROR_READ_NOT_SPECIFIED,
  STREAM_ERROR_WRITE_NOT_SPECIFIED,
  STREAM_ERROR_EOF_NOT_SPECIFIED,
  STREAM_ERROR_SEEK_NOT_SPECIFIED,

  // Input/output pointers are NULL
  STREAM_ERROR_NULL_POINTER,

  // Input/output pointer to read/write isn't NULL but is somehow invalid.
  // Mostly implementation errors.
  STREAM_ERROR_INVALID_POINTER,

  // Seeking out of the bounds of the stream.
  // Implementations have to set this.
  STREAM_ERROR_OUT_OF_RANGE,
  // Invalid 'whence' argument for seek. This can only be set by the stream's
  // seek implmentation as it should be acceptable to define new 'whence'
  // arguments.
  STREAM_ERROR_INVALID_WHENCE,

  // Implementation error: the context is somehow invalid. Buffer might be
  // NULL, FILE might be NULL, might be EOF, etc.
  STREAM_ERROR_INVALID_CONTEXT,

  // Something has gone horribly wrong. Check the log.
  STREAM_ERROR_FAILURE
} stream_error_t;


typedef struct s_stream stream_t;

struct s_stream {
  allocator_t *alloc;   // Allocator (used only in stream_close)
  stream_mode_t mode;   // Mode (may be read, write, or both)
  stream_error_t error; // Last error encountered

  // Reads/writes length bytes from/to the stream.
  // Guarantees to implementations: pointers are not NULL, stream is not NULL,
  // length is greater than zero.
  size_t (*read)(void * const out, size_t length, stream_t *stream);
  size_t (*write)(const void * const in, size_t length, stream_t *stream);
  // Seeks to a point in the stream and returns the new absolute position in
  // the stream.
  // Guarantees: stream is not NULL.
  // Whence may be something other than SEEK_CUR, SEEK_END, or SEEK_SET.
  // Offset may be zero, in which case this operates just as a 'tell' function.
  off_t (*seek)(stream_t *stream, off_t off, int whence);
  // Returns whether the stream is at its EOF point.
  // This is not checked when reading or writing using stream_ functions. It is
  // there for other code to use when reading.
  // Returns 1 if true, 0 if false, or a negative if an error occurred.
  int (*eof)(stream_t *stream);
  // Closes the stream and does whatever else it needs to.
  // Returns zero on success, nonzero on failure.
  int (*close)(stream_t *stream);

  union {
    struct {
      PHYSFS_File *file;
      char *file_path;
    } pfs;
    void *unknown[STREAM_UNKNOWN_CONTEXT_COUNT];
  } context;
};

  ///
// Allocates a new stream (does nothing else)
stream_t *stream_alloc(stream_mode_t mode, allocator_t *alloc);

stream_t *stream_file_open(const char *filepath, stream_mode_t);

// Reads length bytes from stream into out
size_t stream_read(void * const out, size_t length, stream_t *stream);

int stream_read_uint8(stream_t *stream, uint8_t *out);
int stream_read_uint16(stream_t *stream, uint16_t *out);
int stream_read_uint32(stream_t *stream, uint32_t *out);
int stream_read_uint64(stream_t *stream, uint64_t *out);

int stream_read_sint8(stream_t *stream, int8_t *out);
int stream_read_sint16(stream_t *stream, int16_t *out);
int stream_read_sint32(stream_t *stream, int32_t *out);
int stream_read_sint64(stream_t *stream, int64_t *out);

// Writes length bytes to stream from in.
size_t stream_write(const void * const in, size_t length, stream_t *stream);

int stream_write_uint8(stream_t *stream, uint8_t in);
int stream_write_uint16(stream_t *stream, uint16_t in);
int stream_write_uint32(stream_t *stream, uint32_t in);
int stream_write_uint64(stream_t *stream, uint64_t in);

int stream_write_sint8(stream_t *stream, int8_t in);
int stream_write_sint16(stream_t *stream, int16_t in);
int stream_write_sint32(stream_t *stream, int32_t in);
int stream_write_sint64(stream_t *stream, int64_t in);

// Operates the same as fseeko.
// Changes stream position from the current position to an offset from the
// position specified by whence.
// Returns the new position in the stream.
off_t stream_seek(stream_t *stream, off_t off, int whence);

int stream_eof(stream_t *stream);

// Closes a stream and releases its memory
stream_error_t stream_close(stream_t *stream);

// Resets the stream's position/offset to 0.  Returns 0 on success (not because
// zero means anything, that just happens to be the new position).
S_INLINE off_t stream_rewind(stream_t *stream)
{
  return stream_seek(stream, 0, SEEK_SET);
}

// Operates the same as ftello.
// Returns seek(stream, 0, SEEK_CUR).
S_INLINE off_t stream_tell(stream_t *stream)
{
  return stream_seek(stream, 0, SEEK_CUR);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#include <inline.end>

#endif /* end __SNOW__STREAM_H__ include guard */
