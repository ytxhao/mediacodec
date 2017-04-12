//
// Created by Administrator on 2016/11/17.
//

#ifndef YTXPLAYER_GL_ENGINE_H
#define YTXPLAYER_GL_ENGINE_H

// OpenGL ES 2.0 code

#include <jni.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "../../include/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

void addRendererVideoFrame(jobject obj, char *y, char *u, char *v, int videoWidth,
                           int videoHeight);

void addRendererVideoFrameRGBA(jobject obj, void *pixels, int videoWidth,
                           int videoHeight);

void resetRendererVideoFrame(jobject obj);
int rendererStarted(jobject obj);

#ifdef __cplusplus
}
#endif

class GlEngine {

private:

    //    "a_texCoord" //texcoord 是纹理坐标，在后续的Pixel shader中会用到用来读取纹理颜色
    GLfloat coord_buffer[8] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
    };

    //    "vPosition"
    GLfloat vertice_buffer[8] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
    };

    char *plane[3] = {NULL, NULL, NULL};
    void *pixels = NULL;



    GLuint yTextureId, uTextureId, vTextureId, rgbaTextureId;
    int yHandle, uHandle, vHandle, rgbaHandle;
    int videoWidth = 0;
    int videoHeight = 0;
    int mScreenWidth = 720;
    int mScreenHeight = 1080;

    GLuint vertexShader;
    GLuint pixelShader;

    GLuint gProgram;
    GLuint gvPositionHandle;
    GLuint gCoordHandle;
    Lock mRendererLock;
    bool isFrameRendererFinish;

    Lock mLock;
    GlEngine *glEngine;
    bool isInitComplete;

public:
    GlEngine();

    ~GlEngine();

public:

    void releaseGlEngine();

    bool glEngineInitComplete() {
        return GlEngine::isInitComplete;
    }

    void glSetEngineInitComplete(bool isComplete) {
        GlEngine::isInitComplete = isComplete;
    }

    void notifyRenderer();

    void printGLString(const char *name, GLenum s);

    void checkGlError(const char *op);

    GLuint loadShader(GLenum shaderType, const char *pSource);

    GLuint createProgram(const char *pVertexSource, const char *pFragmentSource);

    bool setupGraphics();

    int isSetupGraphics;

    void buildTextures();

    void drawFrame();

    void drawFrameInit(int videoWidth, int videoHeight);

    bool isDrawFrameInit;

    void addRendererFrame(char *y, char *u, char *v, int videoWidth, int videoHeight);

    void addRendererFrameRGBA(void *pixels, int videoWidth, int videoHeight);

    void resetRendererFrame();

    bool isAddRendererFrameInit;

    void addRendererFrameInit(int videoWidth, int videoHeight);

    void addRendererFrameInitRGBA(int videoWidth, int videoHeight);

    void setAspectRatio();

    void setVideoWidthAndHeight(int videoWidth, int videoHeight);

    void renderFrameTest();

    bool setupGraphicsTest(int w, int h);

    void setScreenWidth(int mScreenWidth);

    void setScreenHeight(int mScreenHeight);

    int getScreenWidth();

    int getScreenHeight();

    void waitRendererFinish();

    void signalRendererFinish();

    void setViewPort(int mSurfaceWidth, int mSurfaceHeight);


};


#endif //YTXPLAYER_GL_ENGINE_H
