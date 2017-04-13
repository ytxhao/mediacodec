
#define LOG_NDEBUG 0
#define TAG "YTX-RENDERER-JNI"

#include <unistd.h>
#include <assert.h>
#include "../../include/ALog-priv.h"
#include "../include/gl_engine.h"



// 获取数组的大小
#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif
#define JNIREG_RENDERER_CLASS "ican/ytx/com/mediacodectest/media/player/render/GraphicRenderer"


static const char gVertexShader[] =
        "attribute vec4 vPosition;\n"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 tc;\n"
                "void main() {\n"
                "gl_Position = vPosition;\n"
                "tc = a_texCoord;\n"
                "}\n";

static const char gFragmentShader[] =
        "precision mediump float;\n"
                "uniform sampler2D tex_rgba;\n"
                "varying vec2 tc;\n"
                "void main() {\n"
                "gl_FragColor = texture2D(tex_rgba, tc);\n"
                "}\n";

static GLfloat coord_buffer[8] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
};

//    "vPosition"
static GLfloat vertice_buffer[8] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
};

static JavaVM *sVm;

GlEngine::GlEngine() {

    yTextureId = 1025;
    uTextureId = 1025;
    vTextureId = 1025;
    rgbaTextureId = 1025;

    gProgram = 0;
    yHandle = -1;
    uHandle = -1;
    vHandle = -1;
    rgbaHandle = -1;

    videoWidth = 0;
    videoHeight = 0;
    mScreenWidth = 720;
    mScreenHeight = 1080;

    isAddRendererFrameInit = false;
    isSetupGraphics = 0;
    isDrawFrameInit = false;
    isFrameRendererFinish = false;


    vertexShader = 0;
    pixelShader = 0;

    gvPositionHandle = 0;
    gCoordHandle = 0;

    glEngine = NULL;
    isInitComplete = false;

    pixels = NULL;

    plane[0] = NULL;
    plane[1] = NULL;
    plane[2] = NULL;

}

GlEngine::~GlEngine() {

    int i = 0;
    isAddRendererFrameInit = false;


    if (vertexShader) {
        glDeleteShader(vertexShader);
    }

    if (pixelShader) {
        glDeleteShader(pixelShader);
    }

    if (gProgram) {
        glDeleteProgram(gProgram);
    }


    for (i = 0; i < 3; i++) {
        if (plane[i] != NULL) {
            free(plane[i]);
            plane[i] = NULL;
        }
    }
}

void GlEngine::printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    ALOGI("GL %s = %s\n", name, v);
}

void GlEngine::checkGlError(const char *op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        ALOGI("after %s() glError (0x%x)\n", op, error);
    }
}


GLuint GlEngine::loadShader(GLenum shaderType, const char *pSource) {

    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGE("Could not compile shader %d:\n%s\n",
                          shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;

}

GLuint GlEngine::createProgram(const char *pVertexSource, const char *pFragmentSource) {
    GLuint program;
    vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    ALOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;

}


bool GlEngine::setupGraphics() {
    GLint ret = -1;
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    if (gProgram) {
        glDeleteProgram(gProgram);
    }

    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        ALOGE("Could not create program.");
        return false;
    }
    /*
     * get handle for "vPosition" and "a_texCoord"
     */

    ret = glGetAttribLocation(gProgram, "vPosition");
    if (ret == -1) {
        ALOGE("setupGraphics error glGetAttribLocation(gProgram, \"vPosition\") =%d\n", ret);
    }
    checkGlError("glGetAttribLocation");
    gvPositionHandle = (GLuint) ret;


    ret = glGetAttribLocation(gProgram, "a_texCoord");
    if (ret == -1) {
        ALOGE("setupGraphics error GetAttribLocation(gProgram, \"a_texCoord\") = %d\n", ret);
    }
    checkGlError("glGetAttribLocation");
    gCoordHandle = (GLuint) ret;

    rgbaHandle = glGetUniformLocation(gProgram, "tex_rgba");
    checkGlError("glGetUniformLocation rgbaHandle");
    ALOGI("GLProgram rgbaHandle = %d\n", rgbaHandle);
    if (rgbaHandle == -1) {
        ALOGE("Could not get uniform location for rgbaHandle");
    }

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    isSetupGraphics = 1;
    return true;
}

void GlEngine::setVideoWidthAndHeight(int videoWidth, int videoHeight) {
    this->videoWidth = videoWidth;
    this->videoHeight = videoHeight;
}

void GlEngine::buildTextures() {

    if (rgbaTextureId == 1025) {
        glGenTextures(1, &rgbaTextureId);  //参数1:用来生成纹理的数量. 参数2:存储纹理索引的第一个元素指针
        checkGlError("glGenTextures");
        ALOGI("buildTextures rgbaTextureId=%d\n", rgbaTextureId);
    } else {
        glDeleteTextures(1, &rgbaTextureId);
        checkGlError("glDeleteTextures");

        glGenTextures(1, &rgbaTextureId);
        checkGlError("glGenTextures");
        ALOGI("buildTextures rgbaTextureId=%d\n", rgbaTextureId);
    }


    glBindTexture(GL_TEXTURE_2D, rgbaTextureId);
    checkGlError("glBindTexture");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


}

void GlEngine::drawFrameInit(int videoWidth, int videoHeight) {
    if (!isDrawFrameInit) {
        isDrawFrameInit = true;
        buildTextures();
    }
}

/**
 * render the frame
 * the YUV data will be converted to RGB by shader.
 */
void GlEngine::drawFrame() {
    mLock.lock();
    if (pixels != NULL && videoWidth != 0 && videoHeight != 0) {

        setAspectRatio();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, false, 8, vertice_buffer);
        checkGlError("glVertexAttribPointer mPositionHandle");
        glEnableVertexAttribArray(gvPositionHandle);

        glVertexAttribPointer(gCoordHandle, 2, GL_FLOAT, false, 8, coord_buffer);
        checkGlError("glVertexAttribPointer maTextureHandle");
        glEnableVertexAttribArray(gCoordHandle);

        // bind textures
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, rgbaTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        checkGlError("glTexImage2D");
        glUniform1i(rgbaHandle, 0);


        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glFinish();

        glDisableVertexAttribArray(gvPositionHandle);
        glDisableVertexAttribArray(gCoordHandle);

    }
    mLock.unlock();

}

void GlEngine::signalRendererFinish() {

    mRendererLock.lock();
    isFrameRendererFinish = true;
    mRendererLock.condSignal();
    mRendererLock.unlock();
}

void GlEngine::waitRendererFinish() {
    mRendererLock.lock();
    while (!isFrameRendererFinish) {
        mRendererLock.condWait();
    }
    isFrameRendererFinish = false;
    mRendererLock.unlock();
}


void GlEngine::addRendererFrame(char *y, char *u, char *v, int videoWidth,
                                int videoHeight) {
    mLock.lock();
    addRendererFrameInit(videoWidth, videoHeight);

    memcpy(plane[0], y, (size_t) (videoWidth * videoHeight));
    memcpy(plane[1], u, (size_t) (videoWidth * videoHeight) / 4);
    memcpy(plane[2], v, (size_t) (videoWidth * videoHeight) / 4);
    mLock.unlock();
}


void GlEngine::addRendererFrameRGBA(void *pixels, int videoWidth, int videoHeight) {

    mLock.lock();
    addRendererFrameInitRGBA(videoWidth,videoHeight);

    memcpy(this->pixels, pixels, (size_t) (videoWidth * videoHeight * 4));
    mLock.unlock();
}



/**
 * 清除数据为全黑色
 */
void GlEngine::resetRendererFrame() {
    mLock.lock();

    if (plane[0] != NULL) {
        memset(plane[0], 0, (size_t) (videoWidth * videoHeight));
    }

    if (plane[1] != NULL) {
        memset(plane[1], 128, (size_t) (videoWidth * videoHeight) / 4);
    }

    if (plane[2] != NULL) {
        memset(plane[2], 128, (size_t) (videoWidth * videoHeight) / 4);
    }
    mLock.unlock();

}

void GlEngine::addRendererFrameInit(int videoWidth, int videoHeight) {

    if ((this->videoWidth == 0 && this->videoHeight == 0 && videoWidth != 0)
        || this->videoWidth != videoWidth || this->videoHeight != videoHeight) {
        this->videoWidth = videoWidth;
        this->videoHeight = videoHeight;
        ALOGI("addRendererFrameInit isAddRendererFrameInit");
        if (plane[0] == NULL) {
            plane[0] = (char *) malloc(sizeof(char) * videoWidth * videoHeight);
            plane[1] = (char *) malloc(sizeof(char) * videoWidth * videoHeight / 4);
            plane[2] = (char *) malloc(sizeof(char) * videoWidth * videoHeight / 4);
        } else {
            char *plane_tmp[3];
            plane_tmp[0] = plane[0];
            plane_tmp[1] = plane[1];
            plane_tmp[2] = plane[2];
            plane[0] = (char *) malloc(sizeof(char) * videoWidth * videoHeight);
            plane[1] = (char *) malloc(sizeof(char) * videoWidth * videoHeight / 4);
            plane[2] = (char *) malloc(sizeof(char) * videoWidth * videoHeight / 4);

            free(plane_tmp[0]);
            free(plane_tmp[1]);
            free(plane_tmp[2]);
        }

    }
}

void GlEngine::addRendererFrameInitRGBA(int videoWidth, int videoHeight) {
    if ((this->videoWidth == 0 && this->videoHeight == 0)
        || (this->videoWidth != videoWidth || this->videoHeight != videoHeight)){
        this->videoWidth = videoWidth;
        this->videoHeight = videoHeight;

        if(pixels == NULL){
            pixels = malloc(sizeof(char) * videoWidth * videoHeight * 4);
        }else{
            void *pixels_tmp;
            pixels_tmp = pixels;
            pixels = malloc(sizeof(char) * videoWidth * videoHeight * 4);
            free(pixels_tmp);
        }
    }

}

void GlEngine::setAspectRatio() {
    float f1 = (float) mScreenHeight / mScreenWidth;
    float f2 = (float) videoHeight / videoWidth;
    float widthScale = 0.0;
    float heightScale = 0.0;

    if (f1 == f2) {

    } else if (f1 < f2) {
        widthScale = f1 / f2;
        vertice_buffer[0] = -widthScale;
        vertice_buffer[1] = -1.0f;
        vertice_buffer[2] = widthScale;
        vertice_buffer[3] = -1.0f;
        vertice_buffer[4] = -widthScale;
        vertice_buffer[5] = 1.0f;
        vertice_buffer[6] = widthScale;
        vertice_buffer[7] = 1.0f;

    } else if (f1 > f2) {
        heightScale = f2 / f1;
        vertice_buffer[0] = -1.0f;
        vertice_buffer[1] = -heightScale;
        vertice_buffer[2] = 1.0f;
        vertice_buffer[3] = -heightScale;
        vertice_buffer[4] = -1.0f;
        vertice_buffer[5] = heightScale;
        vertice_buffer[6] = 1.0f;
        vertice_buffer[7] = heightScale;
    }

}


void GlEngine::releaseGlEngine() {
    if (glEngine != NULL) {
        mLock.lock();
        if (glEngine != NULL) {
            GlEngine::glSetEngineInitComplete(false);
            delete glEngine;
            glEngine = NULL;
        }
        mLock.unlock();
    }
}

void GlEngine::setScreenWidth(int mScreenWidth) {
    this->mScreenWidth = mScreenWidth;
}

void GlEngine::setScreenHeight(int mScreenHeight) {
    this->mScreenHeight = mScreenHeight;
}

int GlEngine::getScreenWidth() {
    return this->mScreenWidth;
}

int GlEngine::getScreenHeight() {
    return this->mScreenHeight;
}

void GlEngine::setViewPort(int mSurfaceWidth, int mSurfaceHeight) {
    glViewport(0, 0, mSurfaceWidth, mSurfaceHeight);
    checkGlError("glViewport");
}


static jfieldID context;

static GlEngine *getGlEngine(JNIEnv *env, jobject thiz) {
    GlEngine *const p = (GlEngine *) env->GetIntField(thiz, context);
    return (p);
}

static GlEngine *setGlEngine(JNIEnv *env, jobject thiz, const GlEngine *glEngine) {
    GlEngine *old = (GlEngine *) env->GetIntField(thiz, context);
    env->SetIntField(thiz, context, (int) glEngine);
    return old;
}


void addRendererVideoFrame(jobject obj, char *y, char *u, char *v, int videoWidth,
                           int videoHeight) {

    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    getGlEngine(env, obj)->addRendererFrame(y, u, v, videoWidth, videoHeight);

    sVm->DetachCurrentThread();

}

void addRendererVideoFrameRGBA(jobject obj, void *pixels, int videoWidth,
                               int videoHeight){

    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    getGlEngine(env, obj)->addRendererFrameRGBA(pixels, videoWidth, videoHeight);

    sVm->DetachCurrentThread();

}

void resetRendererVideoFrame(jobject obj) {

    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    getGlEngine(env, obj)->resetRendererFrame();

    sVm->DetachCurrentThread();

}

int rendererStarted(jobject obj) {
    ALOGI("rendererCanStart IN\n");

    int ret = -1;
    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    ret = getGlEngine(env, obj)->isSetupGraphics;

    sVm->DetachCurrentThread();
    ALOGI("rendererCanStart OUT ret=%d\n", ret);
    return ret;
}


JNIEXPORT void JNICALL android_media_player_GraphicRenderer_native_init_opengl
        (JNIEnv *env, jclass clazz) {
    ALOGI("native_1init_1opengl IN");
    context = env->GetFieldID(clazz, "mNativeRenderContext", "I");
    if (context == NULL) {
        return;
    }
    ALOGI("native_1init_1opengl OUT");
}


JNIEXPORT void JNICALL android_media_player_GraphicRenderer_native_constructor_opengl
        (JNIEnv *env, jobject obj, jobject GraphicRenderer_obj) {
    ALOGI("native_1constructor_1opengl IN");

    GlEngine *mGlEngine = new GlEngine();
    setGlEngine(env, obj, mGlEngine);

    ALOGI("native_1constructor_1opengl OUT");

}


// onSurfaceChanged
JNIEXPORT void JNICALL android_media_player_GraphicRenderer_native_resize_opengl
        (JNIEnv *env, jobject obj, jint width, jint height) {
    ALOGI("native_1resize_1opengl IN");

    getGlEngine(env, obj)->setScreenHeight(height);
    getGlEngine(env, obj)->setScreenWidth(width);
    getGlEngine(env, obj)->setViewPort(width, height);

    getGlEngine(env, obj)->buildTextures();
    ALOGI("native_1resize_1opengl OUT");
}
// onDrawFrame
JNIEXPORT void JNICALL android_media_player_GraphicRenderer_native_step_opengl
        (JNIEnv *env, jobject obj) {

    getGlEngine(env, obj)->drawFrame();

}

// onSurfaceCreated
JNIEXPORT void JNICALL android_media_player_GraphicRenderer_native_create_opengl
        (JNIEnv *env, jobject obj) {

    ALOGI("native_create_opengl IN");
    getGlEngine(env, obj)->setupGraphics();
    ALOGI("native_create_opengl OUT");

}



// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {

        {"native_init_opengl",        "()V",                   (void *) android_media_player_GraphicRenderer_native_init_opengl},
        {"native_resize_opengl",      "(II)V",                 (void *) android_media_player_GraphicRenderer_native_resize_opengl},
        {"native_step_opengl",        "()V",                   (void *) android_media_player_GraphicRenderer_native_step_opengl},
        {"native_create_opengl",      "()V",                   (void *) android_media_player_GraphicRenderer_native_create_opengl},
        {"native_constructor_opengl", "(Ljava/lang/Object;)V", (void *) android_media_player_GraphicRenderer_native_constructor_opengl}

};


// 注册native方法到java中
static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


int register_android_media_player_renderer(JNIEnv *env) {
    // 调用注册方法
    return registerNativeMethods(env, JNIREG_RENDERER_CLASS,
                                 gMethods, NELEM(gMethods));
}


jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jint result = -1;
    sVm = vm;

    if (sVm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR renderer: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_media_player_renderer(env) < 0) {
        ALOGE("ERROR: mediaPlayer renderer native registration failed\n");
        goto bail;
    }
    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

    bail:
    return result;
}