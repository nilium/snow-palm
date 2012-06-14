#ifndef __SNOW_BUFFER_STREAM_H__
#define __SNOW_BUFFER_STREAM_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>
#include <stream/stream.h>

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

stream_t *buffer_stream(buffer_t *buffer, stream_mode_t mode, bool destroy_on_close);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW_BUFFER_STREAM_H__ include guard */
