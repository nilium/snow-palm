#define __SNOW__SCENE_C__

#include "scene.h"
#include <entity.h>
// #include <camera.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern entity_t *entity_new(scene_t *scene, const char *name, entity_t *parent);

#ifdef __cplusplus
}
#endif // __cplusplus
