#include "glImpl.h"
#include "gfx_device.h"
#include <cstdlib>

extern gfx_state *g_state;

static sbuffer<gfx_texture> sharedTextures;

gfx_texture *getTexture(GLuint name) {
    if (g_state->flags & CAELINA_SHARED_TEXTURES) {
        for(unsigned int i = 0; i < sharedTextures.size(); i++) {
            if(sharedTextures[i].tname == name) {
                return &sharedTextures[i];
            }
        }
    } else {
        for(unsigned int i = 0; i < g_state->textures.size(); i++) {
            if(g_state->textures[i].tname == name) {
              return &g_state->textures[i];
            }
        }
    }

    return nullptr;
}

extern "C"
{

#ifndef DISABLE_LISTS
gfx_display_list *getList(GLuint name);
#endif

GLboolean glIsTexture( GLuint texture ) {
    CHECK_NULL(g_state, GL_FALSE);

    if(texture == 0) return GL_FALSE;

    if (g_state->flags & CAELINA_SHARED_TEXTURES) {
        for(unsigned int i = 0; i < sharedTextures.size(); i++) {
            if(sharedTextures[i].tname == texture && sharedTextures[i].target != 0) {
                return GL_TRUE;
            }
        }
    } else {
        for(unsigned int i = 0; i < g_state->textures.size(); i++) {
            if(g_state->textures[i].tname == texture && g_state->textures[i].target != 0) {
                return GL_TRUE;
            }
        }
    }

    return GL_FALSE;

}

void glGenTextures( GLsizei n, GLuint *textures ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_ERRORS
    if(n < 0) {
        setError(GL_INVALID_VALUE);
        return;
    }
#endif

    if (g_state->flags & CAELINA_SHARED_TEXTURES) {
        for(GLsizei i = 0; i < n; ++i) {
            GLuint tname = rand() + 1;
            while(sharedTextures.contains(gfx_texture(tname))) {
                tname = rand() + 1;
            }
            textures[i] = tname;

            sharedTextures.push(gfx_texture(tname));
        }
    } else {
      for(GLsizei i = 0; i < n; ++i) {
          GLuint tname = rand() + 1;
          while(g_state->textures.contains(gfx_texture(tname))) {
              tname = rand() + 1;
          }
          textures[i] = tname;

          g_state->textures.push(gfx_texture(tname));
      }
    }
}

void glDeleteTextures( GLsizei n, const GLuint *textures) {
    CHECK_NULL(g_state);

#ifndef DISABLE_ERRORS
    if (n < 0) {
        setError(GL_INVALID_VALUE);
        return;
    }
#endif


    for (GLsizei i = 0; i < n; ++i) {
        gfx_texture *text = getTexture(textures[i]);
        if (text) {
            g_state->device->free_texture(*text);
        }

        if (g_state->flags & CAELINA_SHARED_TEXTURES) {
            sharedTextures.erase(text);
        } else {
            g_state->textures.erase(text);
        }

        if (textures[i] == g_state->currentBoundTexture) {
            g_state->currentBoundTexture = 0;
        }
    }

}

void glBindTexture( GLenum target, GLuint texture ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_LISTS
    if (g_state->withinNewEndListBlock && g_state->displayListCallDepth == 0) {
        gfx_command comm;
        comm.type = gfx_command::BIND_TEXTURE;
        comm.enum1 = target;
        comm.uint1 = texture;
        getList(g_state->currentDisplayList)->commands.push_back(comm);
    }

    CHECK_COMPILE_AND_EXECUTE(g_state);
#endif

    gfx_texture *text = getTexture(texture);

    if(!text) {
        text = new gfx_texture(texture, target);
        if (g_state->flags & CAELINA_SHARED_TEXTURES) {
            sharedTextures.push(*text);
        } else {
            g_state->textures.push(*text);
        }
        glBindTexture(target, texture);
        return;
    }

#ifndef DISABLE_ERRORS
    if(target != GL_TEXTURE_2D
#if !defined(SPEC_GLES) || defined(SPEC_GLES2)
       && target != GL_TEXTURE_CUBE_MAP
#endif
       ) {
        setError(GL_INVALID_ENUM);
        return;
    }
#endif
    
    if(text->target == 0 ) {
        text->target = target;
        
    }
#ifndef DISABLE_ERRORS
    else if(text->target != target) {
        setError(GL_INVALID_OPERATION);
        return;
    }
#endif

    g_state->currentBoundTexture = text->tname;
}


void glTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_LISTS
    if (g_state->withinNewEndListBlock && g_state->displayListCallDepth == 0) {
        gfx_command comm;
        comm.type = gfx_command::TEX_IMAGE_2D;
        comm.enum1 = target;
        comm.int1 = level;
        comm.int2 = internalFormat;
        comm.size1 = width;
        comm.size2 = height;
        comm.int2 = border;
        comm.enum2 = format;
        comm.enum3 = type;
        comm.voidp = (GLvoid *)pixels;
        getList(g_state->currentDisplayList)->commands.push_back(comm);
    }

    CHECK_COMPILE_AND_EXECUTE(g_state);
#endif

#ifndef DISABLE_ERRORS
    if(target != GL_TEXTURE_2D) {
        setError(GL_INVALID_ENUM);
        return;
    }

    switch(format) {
        case (GL_ALPHA):
        case (GL_RGB):
        case (GL_RGBA):
#ifndef SPEC_GLES
        case (GL_BGRA):
#else
        case (GL_BGRA_EXT):
#endif
        case (GL_LUMINANCE):
        case (GL_LUMINANCE_ALPHA): {

        } break;

        default: {
            setError(GL_INVALID_VALUE);
            return;
        } break;

    }

    switch(type) {
        case (GL_UNSIGNED_BYTE):
        case (GL_UNSIGNED_SHORT_5_6_5):
        case (GL_UNSIGNED_SHORT_4_4_4_4):
        case (GL_UNSIGNED_SHORT_5_5_5_1): {

        } break;

        default: {
            setError(GL_INVALID_ENUM);
            return;
        } break;

    }

    if(level < 0 || level > log2(IMPL_MAX_TEXTURE_SIZE)) {
        setError(GL_INVALID_VALUE);
        return;
    }

    //Not really needed, duplicate functionailty of switch(format). Here for spec reasons.
    switch(internalFormat) {
        case (GL_ALPHA):
        case (GL_RGB):
        case (GL_RGBA):
#ifndef SPEC_GLES
        case (GL_BGRA):
#else
        case (GL_BGRA_EXT):
#endif
        case (GL_LUMINANCE):
        case (GL_LUMINANCE_ALPHA): {

        } break;

        default: {
            setError(GL_INVALID_VALUE);
            return;
        } break;

    }

    if(width < 0 || height < 0
       || width > IMPL_MAX_TEXTURE_SIZE
       || height > IMPL_MAX_TEXTURE_SIZE
       || width &  0x00000001
       || height & 0x00000001) {
        setError(GL_INVALID_VALUE);
        return;
    }

    if(border != 0) {
        setError(GL_INVALID_VALUE);
        return;
    }

    if((GLenum)internalFormat != format) {
        setError(GL_INVALID_OPERATION);
        return;
    }

    if(type == GL_UNSIGNED_SHORT_5_6_5
       && format != GL_RGB) {
        setError(GL_INVALID_OPERATION);
        return;
    }

    if((type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1)
       && (format != GL_RGBA
#ifndef SPEC_GLES
       || format != GL_BGRA
#else
       || format != GL_BGRA_EXT
#endif
           ) ) {
        setError(GL_INVALID_OPERATION);
        return;
    }
#endif

    gfx_texture* text = getTexture(g_state->currentBoundTexture);

    if (!text) return;

    text->unpackedColorBuffer = (GLubyte*)linearAlloc(sizeof(GLubyte) * 4 * width * height);
    text->width = width;
    text->height = height;
    text->format = format;

    if(pixels) {
        unsigned int accum = 0;
        //TODO implement unpacking for more texture formats
        for(GLsizei y = 0; y < height; y++) {
            for(GLsizei x = 0; x < width; x++) {
                switch(format) {
                    case (GL_RGBA): {
                        if(type == GL_UNSIGNED_BYTE) {
                            int index = (x + y * width) * 4;
                            GLubyte* bpixels = (GLubyte*)pixels;
                            text->unpackedColorBuffer[index] = bpixels[accum];
                            text->unpackedColorBuffer[index + 1] = bpixels[accum + 1];
                            text->unpackedColorBuffer[index + 2] = bpixels[accum + 2];
                            text->unpackedColorBuffer[index + 3] = bpixels[accum + 3];
                            accum += 4;
                        }
                        else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                            int index = (x + y * width) * 4;
                            GLushort pixel = ((GLushort*)pixels)[accum];
                            float R = (float)((pixel >> 11) & 0b11111) / 31.0f;
                            float G = (float)((pixel >> 6) & 0b11111) / 31.0f;
                            float B = (float)((pixel >> 1) & 0b11111) / 31.0f;
                            text->unpackedColorBuffer[index + 0] = (GLubyte)(R *  255.0f);
                            text->unpackedColorBuffer[index + 1] = (GLubyte)(G *  255.0f);
                            text->unpackedColorBuffer[index + 2] = (GLubyte)(B *  255.0f);
                            text->unpackedColorBuffer[index + 3] = 0xFF * (pixel & 1);
                            accum ++;
                        }
                    } break;
#ifndef SPEC_GLES
                  case (GL_BGRA):
#else
                  case (GL_BGRA_EXT):
#endif
                  {
                    if(type == GL_UNSIGNED_BYTE) {
                      int index = (x + y * width) * 4;
                      GLubyte* bpixels = (GLubyte*)pixels;
                      text->unpackedColorBuffer[index + 3] = bpixels[accum + 0];
                      text->unpackedColorBuffer[index + 2] = bpixels[accum + 1];
                      text->unpackedColorBuffer[index + 1] = bpixels[accum + 2];
                      text->unpackedColorBuffer[index + 0] = bpixels[accum + 3];
                      accum += 4;
                    }
                    else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                      int index = (x + y * width) * 4;
                      GLushort pixel = ((GLushort*)pixels)[accum];
                      float R = (float)((pixel >> 11) & 0b11111) / 31.0f;
                      float G = (float)((pixel >> 6) & 0b11111) / 31.0f;
                      float B = (float)((pixel >> 1) & 0b11111) / 31.0f;
                      text->unpackedColorBuffer[index + 3] = (GLubyte)(R *  255.0f);
                      text->unpackedColorBuffer[index + 2] = (GLubyte)(G *  255.0f);
                      text->unpackedColorBuffer[index + 1] = (GLubyte)(B *  255.0f);
                      text->unpackedColorBuffer[index + 0] = 0xFF * (pixel & 1);
                      accum ++;
                    }
                  } break;

                    case (GL_ALPHA): {
                        if(type == GL_UNSIGNED_BYTE) {
                            int index = (x + y * width) * 4;
                            GLubyte* bpixels = (GLubyte*)pixels;
                            text->unpackedColorBuffer[index] = 0x00;
                            text->unpackedColorBuffer[index + 1] = 0x00;
                            text->unpackedColorBuffer[index + 2] = 0x00;
                            text->unpackedColorBuffer[index + 3] = bpixels[accum];
                            accum++;
                        }
                    } break;
#ifndef DISABLE_ERRORS
                    default: {

                        setError(GL_INVALID_VALUE);
                        return;
                    } break;
#endif
                }
            }

            while(accum % g_state->unpackAlignment != 0) {
                accum++;
            }
        }
    }
    g_state->device->repack_texture(*text);
}

void glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_ERRORS
    if(target != GL_TEXTURE_2D) {
        setError(GL_INVALID_ENUM);
        return;
    }

    switch (format) {
#ifndef SPEC_GLES
        case GL_COLOR_INDEX:
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
#endif
        case GL_ALPHA:
        case GL_RGB:
        case GL_RGBA:
#ifndef SPEC_GLES
        case (GL_BGRA):
#else
        case (GL_BGRA_EXT):
#endif
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA: {

        } break;

        default: {
            setError(GL_INVALID_ENUM);
            return;
        } break;
    }

    switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
#ifndef SPEC_GLES
        case GL_BITMAP:
#endif
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_SHORT:
        case GL_UNSIGNED_INT:
#if !defined(SPEC_GLES) || defined(SPEC_GLES2)
        case GL_INT:
#endif
        case GL_FLOAT: {

        } break;

        default: {
            setError(GL_INVALID_ENUM);
            return;
        } break;
    }
#ifndef SPEC_GLES
    if (type == GL_BITMAP && format != GL_COLOR_INDEX) {
        setError(GL_INVALID_ENUM);
        return;
    }
#endif

#endif // DISABLE_ERRORS

    gfx_texture* text = getTexture(g_state->currentBoundTexture);

#ifndef DISABLE_ERRORS
    if (!text) {
        setError(GL_INVALID_OPERATION);
        return;
    }
#endif

    if(pixels) {
        unsigned int accum = 0;
        //TODO implement unpacking for more texture formats
        for(GLsizei y = yoffset; y < yoffset + height && y < text->height; ++y) {
            for(GLsizei x = xoffset; x < yoffset + width && x < text->width; ++x) {
                switch(format) {
                    case (GL_RGBA): {
                        if(type == GL_UNSIGNED_BYTE) {
                            int index = (x + y * text->width) * 4;
                            int srcindex = ((x - xoffset) + y * width) * 4;
                            GLubyte* bpixels = (GLubyte*)pixels;
                            text->unpackedColorBuffer[index] = bpixels[srcindex];
                            text->unpackedColorBuffer[index + 1] = bpixels[srcindex + 1];
                            text->unpackedColorBuffer[index + 2] = bpixels[srcindex + 2];
                            text->unpackedColorBuffer[index + 3] = bpixels[srcindex + 3];
                            accum += 4;
                        }
                        else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                            int index = (x + y * width) * 4;
                            GLushort pixel = ((GLushort*)pixels)[accum];
                            float R = (float)((pixel >> 11) & 0b11111) / 31.0f;
                            float G = (float)((pixel >> 6) & 0b11111) / 31.0f;
                            float B = (float)((pixel >> 1) & 0b11111) / 31.0f;
                            text->unpackedColorBuffer[index + 0] = (GLubyte)(R *  255.0f);
                            text->unpackedColorBuffer[index + 1] = (GLubyte)(G *  255.0f);
                            text->unpackedColorBuffer[index + 2] = (GLubyte)(B *  255.0f);
                            text->unpackedColorBuffer[index + 3] = 0xFF * (pixel & 1);
                            accum ++;
                        }
                    } break;

#ifndef SPEC_GLES
                  case (GL_BGRA):
#else
                  case (GL_BGRA_EXT):
#endif
                  {
                    if(type == GL_UNSIGNED_BYTE) {
                      int index = (x + y * width) * 4;
                      GLubyte* bpixels = (GLubyte*)pixels;
                      text->unpackedColorBuffer[index + 3] = bpixels[accum + 0];
                      text->unpackedColorBuffer[index + 2] = bpixels[accum + 1];
                      text->unpackedColorBuffer[index + 1] = bpixels[accum + 2];
                      text->unpackedColorBuffer[index + 0] = bpixels[accum + 3];
                      accum += 4;
                    }
                    else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                      int index = (x + y * width) * 4;
                      GLushort pixel = ((GLushort*)pixels)[accum];
                      float R = (float)((pixel >> 11) & 0b11111) / 31.0f;
                      float G = (float)((pixel >> 6) & 0b11111) / 31.0f;
                      float B = (float)((pixel >> 1) & 0b11111) / 31.0f;
                      text->unpackedColorBuffer[index + 3] = (GLubyte)(R *  255.0f);
                      text->unpackedColorBuffer[index + 2] = (GLubyte)(G *  255.0f);
                      text->unpackedColorBuffer[index + 1] = (GLubyte)(B *  255.0f);
                      text->unpackedColorBuffer[index + 0] = 0xFF * (pixel & 1);
                      accum ++;
                    }
                  } break;

                    case (GL_ALPHA): {
                        if(type == GL_UNSIGNED_BYTE) {
                            int index = (x + y * text->width) * 4;
                            int srcindex = ((x - xoffset) + y * width);
                            GLubyte* bpixels = (GLubyte*)pixels;
                            text->unpackedColorBuffer[index] = 0x00;
                            text->unpackedColorBuffer[index + 1] = 0x00;
                            text->unpackedColorBuffer[index + 2] = 0x00;
                            text->unpackedColorBuffer[index + 3] = bpixels[srcindex];
                            accum++;
                        }
                    } break;
                        
                }
            }
            
            while(accum % g_state->unpackAlignment != 0) {
                accum++;
            }
        }
    }
    
    g_state->device->repack_texture(*text);
}

void glPixelStorei( GLenum pname, GLint param ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_ERRORS
    if(param != 1 && param != 2 && param != 4 && param != 8) {
        setError(GL_INVALID_VALUE);
        return;
    }
#endif

    switch(pname) {
        case (GL_PACK_ALIGNMENT): {
            g_state->packAlignment = param;
        } break;
        case (GL_UNPACK_ALIGNMENT): {
            g_state->unpackAlignment = param;
        } break;
#ifndef DISABLE_ERRORS
        default: {
            setError(GL_INVALID_ENUM);
            return;
        }
#endif
    }
    
}


static GPU_TEXTURE_WRAP_PARAM gl_tex_wrap(GLenum wrap) {
    switch (wrap) {
        case GL_CLAMP_TO_EDGE: return GPU_CLAMP_TO_EDGE;

#if !defined(SPEC_GLES) || defined(SPEC_GLES2)
        case GL_MIRRORED_REPEAT: return GPU_MIRRORED_REPEAT;
#else
        case GL_MIRRORED_REPEAT_OES: return GPU_MIRRORED_REPEAT;
#endif

#ifndef SPEC_GLES
        case GL_CLAMP_TO_BORDER: return GPU_CLAMP_TO_BORDER;
#elif defined(SPEC_GLES2)
        case GL_CLAMP_TO_BORDER_OES: return GPU_CLAMP_TO_BORDER;
#endif
        case GL_REPEAT: return GPU_REPEAT;
    }

    return GPU_REPEAT;
}

static GPU_TEXTURE_FILTER_PARAM gl_tex_filter(GLenum filt) {
    switch (filt) {
        case GL_LINEAR: return GPU_LINEAR;
        case GL_NEAREST: return GPU_NEAREST;
    }
    
    return GPU_LINEAR;
}

void glTexParameteri( GLenum target, GLenum pname, GLint param ) {
    CHECK_NULL(g_state);

#ifndef DISABLE_LISTS
    if (g_state->withinNewEndListBlock && g_state->displayListCallDepth == 0) {
        gfx_command comm;
        comm.type = gfx_command::TEX_PARAM_I;
        comm.enum1 = target;
        comm.enum2 = pname;
        comm.int1 = param;
        getList(g_state->currentDisplayList)->commands.push_back(comm);
    }

    CHECK_COMPILE_AND_EXECUTE(g_state);
#endif

#ifndef DISABLE_ERRORS
    switch (target) {
        case GL_TEXTURE_2D:
#if !defined(SPEC_GLES) || defined(SPEC_GLES2)
        case GL_TEXTURE_CUBE_MAP:
#endif
        {

        } break;

        default: {
            setError(GL_INVALID_ENUM);
            return;
        }
    }

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER: {
            switch (param) {
                case GL_LINEAR:
                case GL_NEAREST: {

                } break;

                default: {
                    setError(GL_INVALID_ENUM);
                    return;
                } break;
            }
        } break;
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T: {
            switch (param) {
                case GL_CLAMP_TO_EDGE:
#ifndef SPEC_GLES
              case GL_CLAMP_TO_BORDER:
#elif defined(SPEC_GLES2)
              case GL_CLAMP_TO_BORDER_OES:
#endif
#if !defined(SPEC_GLES) || defined(SPEC_GLES2)
                case GL_MIRRORED_REPEAT:
#else
                case GL_MIRRORED_REPEAT_OES:
#endif
                case GL_REPEAT: {
                } break;

                default: {
                    setError(GL_INVALID_ENUM);
                    return;
                } break;
            }
        } break;

        default: {
            setError(GL_INVALID_ENUM);
            return;
        }
    }
#endif

    gfx_texture* text = getTexture(g_state->currentBoundTexture);

    if (!text) return;

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER: {
            text->min_filter = gl_tex_filter(param);
        } break;
        case GL_TEXTURE_MAG_FILTER: {
            text->mag_filter = gl_tex_filter(param);
        } break;
        case GL_TEXTURE_WRAP_S: {
            text->wrap_s = gl_tex_wrap(param);
        } break;
        case GL_TEXTURE_WRAP_T: {
            text->wrap_t = gl_tex_wrap(param);
        } break;
            
    }
}

}