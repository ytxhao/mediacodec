//
// Created by Administrator on 2017/4/11.
//

#ifndef MEDIACODECTEST_GLTHREAD_H
#define MEDIACODECTEST_GLTHREAD_H

#include "VideoStateInfo.h"
#include "Thread.h"
#include "headerBmp.h"


#define GL_MSG_CANCEL               0
#define GL_MSG_RENDERER             1
#define GL_MSG_DECODED_FIRST_FRAME   2

class GLThread : public Thread {

public:
    GLThread(VideoStateInfo *mVideoStateInfo);

    ~GLThread();

    void handleRun(void *ptr);

    bool prepare();

    void refresh();

    bool process(AVMessage *msg);

    void stop();

    void initEGL();
    void drawGL(GlslFilter *filter);
    void deInitEGL();
    void enqueue(AVMessage *msg);
    void saveBmp();
    void SnapshotBmpRGBA(GLvoid * pData, int width, int height,  char * filename);
    void SnapshotBmpRGB(GLvoid * pData, int width, int height,  char * filename);

    VideoStateInfo *mVideoStateInfo = NULL;
    GlslFilter *glslFilter=NULL;
    Picture *mPicture;
    Picture *mTexturePicture;
    int texture;

    EGLConfig eglConf;
    EGLSurface eglSurface;
    EGLContext eglCtx;
    EGLDisplay eglDisp;

    int times=0;

};

#endif //MEDIACODECTEST_GLTHREAD_H
