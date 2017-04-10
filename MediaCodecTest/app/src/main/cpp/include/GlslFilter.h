//
// Created by Administrator on 2017/4/7.
//

#ifndef MEDIACODECTEST_GLSLFILTER_H
#define MEDIACODECTEST_GLSLFILTER_H


#include <stdint.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "MediaPlayerListener.h"
#include <png.h>
// ----------------------------------------------------------------------------
// for native window JNI
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>



typedef struct {

    GLuint texture;
    int width;
    int height;

}Picture;


class GlslFilter {

private:
    GLuint createProgram(const char *pVertexSource, const char *pFragmentSource);

public:
    GlslFilter();

    ~GlslFilter();

    void initial();
    void printGLString(const char *name, GLenum s);
    void checkGlError(const char *op);
    GLuint loadShader(GLenum shaderType, const char *pSource);
    void renderBackground();
    void process(Picture *in, Picture *out);
    GLuint createTexture();

    const char *getVertexShaderString();

    const char *getFragmentShaderString();

    bool isInitialed = true;
    GLuint vertexShader;
    GLuint pixelShader;
    GLuint shaderProgram;
    GLuint frameBufferObjectId = 0;//创建一个帧缓冲区对象


    GLint texSamplerHandle;
    GLint texCoordHandle;
    GLint posCoordHandle;
 //   GLuint texCoordMatHandle;
 //   GLuint modelViewMatHandle;

    //GLuint textureOut;

    GLfloat posVertices[8] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
    };

    GLfloat texVertices[8] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
    };

//    GLfloat mTextureMat[16] = {
//            1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
//            1
//    };
//    GLfloat mModelViewMat[16] = {
//            1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
//            0, 1
//    };

};














#endif //MEDIACODECTEST_GLSLFILTER_H
