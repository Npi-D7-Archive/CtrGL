#ifndef GFX_DEVICE_INTERNAL_H
#define GFX_DEVICE_INTERNAL_H

#include "vector.h"
#include "matrix.h"

#ifndef _3DS
typedef int s32;
typedef unsigned char u8;
typedef char s8;
#else
#include <3ds.h>
#endif

struct gfx_state;
struct gfx_texture;
struct gfx_device_3ds;

template <class T>
class sbuffer {
    T* buffer;
    unsigned int buffer_size;
    unsigned int current_index;
    unsigned int reserve_block_size;
public:

    sbuffer(unsigned int reserve = 2048) {
        if(reserve == 0) reserve = 2048;
        buffer = new T[reserve];
        buffer_size = reserve;
        current_index = 0;
        reserve_block_size = reserve;
    }

    ~sbuffer() {
        if(buffer) delete[] buffer;
    }

    void push(const T& data) {
        if(current_index + 1 >= buffer_size) {
            resize(buffer_size + reserve_block_size);
        }

        buffer[current_index] = data;
        current_index++;
    }

    void resize(unsigned int n_size) {
        T* n_buf = new T[n_size];
        for(unsigned int i = 0; i < buffer_size; i++) {
            n_buf[i] = buffer[i];
        }

        delete[] buffer;
        buffer = n_buf;
        buffer_size = n_size;
    }

    void clear() {
        current_index = 0;
    }

    unsigned int size() const {
        return current_index;
    }

    T& operator[](unsigned int index) const {
        return buffer[index];
    }

    bool contains(const T& data) const {
        for(unsigned int i = 0; i < size(); ++i) {
            if(data == buffer[i]) return true;
        }

        return false;
    }

    void erase(T* data) {
        int index = 0;
        for (unsigned int i = 0 ; i < size(); ++i) {
            if (data == &buffer[i]) {
                index = i;
                break;
            }
        }
        for (unsigned int i = index; i < size() - 1; i++) {
            buffer[i] = buffer[i + 1];
        }
        --current_index;
    }
};

struct vertex {
    vec4 position;
    vec4 color;
    vec4 textureCoord;
    vec4 normal;

    vertex(const vec4& pos = vec4(0, 0, 0, 1),
           const vec4& col = vec4(1, 1, 1, 1),
           const vec4& tex = vec4(0, 0, 0, 1),
           const vec4& norm = vec4(0, 0, 1, 0)) {
        position = vec4(pos);
        color = vec4(col);
        textureCoord = vec4(tex);
        normal = vec4(norm);
    }

    vertex(const vertex& v) {
        position = vec4(v.position);
        color = vec4(v.color);
        textureCoord = vec4(v.textureCoord);
        normal = vec4(v.normal);
    }
};


struct gfx_texture {
    GLuint tname;
    GLenum target;
    GLubyte* colorBuffer = NULL;
    GLubyte* unpackedColorBuffer = NULL;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GPU_TEXTURE_FILTER_PARAM min_filter = GPU_LINEAR;
    GPU_TEXTURE_FILTER_PARAM mag_filter = GPU_LINEAR;
    GPU_TEXTURE_WRAP_PARAM wrap_s = GPU_REPEAT;
    GPU_TEXTURE_WRAP_PARAM wrap_t = GPU_REPEAT;

    GLuint extdata = 0;

    gfx_texture(GLuint name = 0, GLenum tar = 0) {
        tname = name;
        target = tar;
        colorBuffer = NULL;
    }

    ~gfx_texture() {
        //if(colorBuffer) delete colorBuffer;
    }


    bool operator==(const gfx_texture& t) const {
        return t.tname == tname;
    }
};

struct gfx_vec4i {
    GLint x;
    GLint y;
    GLint z;
    GLint w;
};

#ifndef DISABLE_LISTS
struct gfx_command {
    enum CMD_TYPE {
        PUSH_MATRIX = 0,
        POP_MATRIX = 1,
        MATRIX_MODE = 2,
        CLEAR_COLOR = 3,
        CLEAR = 4,
        LOAD_IDENTITY = 5,
        BEGIN = 6,
        END = 7,
        BIND_TEXTURE = 8,
        TEX_IMAGE_2D,
        ROTATE,
        SCALE,
        TRANSLATE,
        ORTHO,
        FRUSTUM,
        VIEWPORT,
        BLEND_FUNC,
        ENABLE,
        DISABLE,
        TEX_PARAM_I,
        SCISSOR,
        CALL_LIST,
        LIGHTF,
        LIGHTFV,
        ALPHA_FUNC,
        COLOR_MASK,
        DEPTH_MASK,
        STENCIL_MASK,
        STENCIL_FUNC,
        STENCIL_OP,
        BLEND_COLOR,
        CLEAR_DEPTH,
        DEPTH_FUNC,
        NONE
    };

    CMD_TYPE type = NONE;
    GLuint vdata_size = 0;
    GLuint vdata_units = 0;
    u8 *vdata = NULL;

    GLenum enum1 = 0;
    GLenum enum2 = 0;
    GLenum enum3 = 0;
    GLfloat floats[4];
    GLfloat float5;
    GLfloat float6;
    GLbitfield mask1;
    GLuint uint1;
    GLint int1;
    GLint int2;
    GLint int3;
    GLsizei size1;
    GLsizei size2;
    GLvoid *voidp = NULL;

};

#include <vector>

struct gfx_display_list {
    GLuint name;
    GLboolean useColor = GL_FALSE;
    GLboolean useTex = GL_FALSE;
    GLboolean useNormal = GL_FALSE;
    vec4 vColor;
    vec4 vTex;
    vec4 vNormal;
    std::vector<gfx_command> commands;
};
#endif

struct gfx_light {
    vec4 ambient = { 0.0, 0.0, 0.0, 1.0 };
    vec4 diffuse = { 0.0, 0.0, 0.0, 1.0 }; // set in gfx_device()
    vec4 specular = { 0.0, 0.0, 0.0, 1.0 }; // set int gfx_device();
    vec4 position = { 0.0, 0.0, 1.0, 0.0 };
    vec4 spotlightDirection = { 0.0, 0.0, -1.0, 0.0 };
    float spotlightExpo = 0.0; // [0.0, 128.0]
    float spotlightCutoff = 180.0; // [0.0, 90.0], 180.0
    float constantAttenuation = 1.0; // [0.0, inf]
    float linearAttenuation = 0.0; // [0.0, inf]
    float quadraticAttenuation = 0.0; // [0.0, inf]
};

struct gfx_material {
    vec4 ambientColor = { 0.2, 0.2, 0.2, 1.0 };
    vec4 diffuseColor = { 0.8, 0.8, 0.8, 1.0 };
    vec4 specularColor = { 0.0, 0.0, 0.0, 1.0 };
    vec4 emissiveColor = { 0.0, 0.0, 0.0, 1.0 };
    float specularExpo = 0.0; // [0.0, 128.0]
    float ambientColorIndex = 0.0;
    float diffuseColorIndex = 1.0;
    float specularColorIndex = 1.0;
};

struct gfx_state {
    gfx_device_3ds* device;
    int flags;

    vec4 clearColor;

    mat4 modelviewMatrixStack[IMPL_MAX_MODELVIEW_STACK_DEPTH];
    mat4 projectionMatrixStack[IMPL_MAX_PROJECTION_STACK_DEPTH];
    mat4 textureMatrixStack[IMPL_MAX_TEXTURE_STACK_DEPTH];
    mat4 viewportMatrix;

    s8 currentModelviewMatrix = 0;
    s8 currentProjectionMatrix = 0;
    s8 currentTextureMatrix = 0;

    GLenum matrixMode = GL_MODELVIEW;

#ifndef DISABLE_ERRORS
    GLenum errorFlag;
#endif

    GLboolean withinBeginEndBlock = GL_FALSE;

    sbuffer<vertex> vertexBuffer;
    vec4 currentVertexColor = vec4(1, 1, 1, 1);
    vec4 currentTextureCoord = vec4(0, 0, 0, 1);
    vec4 currentVertexNormal = vec4(0, 0, 1, 0);
    GLenum vertexDrawMode;

    sbuffer<gfx_texture> textures;
    GLuint currentBoundTexture = 0;
    GLint packAlignment = 4;
    GLint unpackAlignment = 4;

    GLenum blendSrcFactor = GL_ONE;
    GLenum blendDstFactor = GL_ZERO;

    GLuint blendColor = 0;

    GLenum alphaTestFunc = GL_ALWAYS;
    GLclampf alphaTestRef = 0.0;

    GLfloat clearDepth = 1.0f;
    GLenum depthFunc = GL_LESS;

    GLboolean colorMaskRed = GL_TRUE;
    GLboolean colorMaskBlue = GL_TRUE;
    GLboolean colorMaskGreen = GL_TRUE;
    GLboolean colorMaskAlpha = GL_TRUE;

    GLboolean depthMask = GL_TRUE;

    GLuint stencilMask = 0xFFFFFFFF;
    GLenum stencilFunc = GL_ALWAYS;
    GLuint stencilFuncMask = 0xFFFFFFFF;
    GLint stencilRef = 0;

    GLenum stencilOpSFail = GL_KEEP;
    GLenum stencilOpZFail = GL_KEEP;
    GLenum stencilOpZPass = GL_KEEP;

    GLboolean enableTexture2D = GL_FALSE;
    GLboolean enableDepthTest = GL_FALSE;
    GLboolean enableBlend = GL_FALSE;
    GLboolean enableScissorTest = GL_FALSE;
    GLboolean enableLighting = GL_FALSE;
    GLboolean enableLight[IMPL_MAX_LIGHTS];
    GLboolean enableAlphaTest = GL_FALSE;
    GLboolean enableStencilTest = GL_FALSE;

    gfx_vec4i scissorBox;

    gfx_light lights[IMPL_MAX_LIGHTS];
    vec4 lightModelAmbient = { 0.2, 0.2, 0.2, 1.0};
    GLboolean lightModelLocalEye = GL_FALSE;
    GLboolean lightModelTwoSided = GL_FALSE; //TODO implement two-sided lighting
    gfx_material material;

#ifndef DISABLE_LISTS
    sbuffer<gfx_display_list> displayLists;
    GLuint nextDisplayListName = 1;
    GLboolean withinNewEndListBlock = GL_FALSE;
    GLenum newDisplayListMode = GL_COMPILE_AND_EXECUTE;
    GLuint currentDisplayList = 0; //for compiling only
    GLuint displayListCallDepth = 0;
    u8 *endVBOData;
    GLsizei endVBOUnits;
#endif

    GLint vertexPtrSize = 0;
    GLenum vertexPtrType = GL_FLOAT;
    GLsizei vertexPtrStride = 0;
    const GLvoid *vertexPtr = nullptr;
};

class gfx_device {
public:
    int width, height;
    gfx_state* g_state = NULL;
    
    gfx_device(gfx_state *state, int w, int h) {
        g_state = state;
        g_state->scissorBox = {0, 0, w, h};
        width = w;
        height = h;
        g_state->lights[0].diffuse = { 1.0, 1.0, 1.0, 1.0 };
        g_state->lights[0].specular = { 1.0, 1.0, 1.0, 1.0 };
        
        for (int i = 0; i < IMPL_MAX_LIGHTS; ++i) {
            g_state->enableLight[i] = GL_FALSE;
        }
    }

    int getWidth() {
        return width;
    }
    
    int getHeight() {
        return height;
    }
};

#include "driver_3ds.h"


inline float min(float a, float b) {
    return a < b ? a : b;
}

inline float clampf(float val, float low, float hi) {
    return val < low ? low : (val > hi ? hi : val);
}

inline int clampi(int val, int low, int hi) {
    return val < low ? low : (val > hi ? hi : val);
}

#endif
