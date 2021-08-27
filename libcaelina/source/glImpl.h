#ifndef GLIMPL_H
#define GLIMPL_H

#define IMPL_MAX_MODELVIEW_STACK_DEPTH    32
#define IMPL_MAX_PROJECTION_STACK_DEPTH    2
#define IMPL_MAX_TEXTURE_STACK_DEPTH       2
#define IMPL_MAX_TEXTURE_SIZE           1024
#define IMPL_MAX_LIST_CALL_DEPTH          64
#define IMPL_MAX_LIGHTS                    8

#ifdef SPEC_GLES
#define DISABLE_LISTS 1
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "gpulib.h"
#include "gfx_device_internal.h"

#define CHECK_NULL(x, ...) \
    if(!x) return __VA_ARGS__;

#ifndef DISABLE_ERRORS
#define CHECK_WITHIN_BEGIN_END(x, ...) \
    if(g_state->withinBeginEndBlock) { \
        setError(GL_INVALID_OPERATION); \
        return __VA_ARGS__; \
    }

#define CHECK_WITHIN_NEW_END(x, ...) \
    if(g_state->withinNewEndListBlock) { \
        setError(GL_INVALID_OPERATION); \
        return __VA_ARGS__; \
    }

#else

#define CHECK_WITHIN_BEGIN_END(x, ...) ((void)0)
#define CHECK_WITHIN_NEW_END(x, ...) ((void)0)
#endif

#define CHECK_COMPILE_AND_EXECUTE(x, ...) \
    if(g_state->newDisplayListMode == GL_COMPILE) { \
        return __VA_ARGS__; \
    }

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_ERRORS
    void setError(GLenum error);
#endif

#ifdef __cplusplus
}
#endif

#endif
