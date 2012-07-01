#ifndef __SNOW__FILE_STREAM_H__
#define __SNOW__FILE_STREAM_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>
#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

stream_t *file_open(const char *path, stream_mode_t mode, allocator_t *alloc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW__FILE_STREAM_H__ include guard */
