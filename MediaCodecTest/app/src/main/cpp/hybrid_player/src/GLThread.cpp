//
// Created by Administrator on 2017/4/11.
//
#define LOG_NDEBUG 0
#define TAG "YTX-GLThread-JNI"
#include <ALog-priv.h>
#include <GlslFilter.h>
#include <android_media_YtxMediaPlayer.h>
#include "GLThread.h"


GLThread::GLThread(VideoStateInfo *mVideoStateInfo) {

    this->mVideoStateInfo = mVideoStateInfo;
    mPicture=NULL;
    mTexturePicture=NULL;

}

void GLThread::handleRun(void *ptr)
{
    if (!prepare()) {
        ALOGI("Couldn't prepare AudioRefreshController\n");
        return;
    }
    refresh();

}

void GLThread::refresh() {
    AVMessage msg;
    while (mRunning) {
        if (mVideoStateInfo->messageQueueGL->get(&msg, true) < 0) {
            mRunning = false;
        } else {
            if (!process(&msg)) {
                mRunning = false;
            }
        }
    }

}

bool GLThread::process(AVMessage *msg) {
    bool ret = true;

    if (msg->what == GL_MSG_CANCEL) {
        deInitEGL();
        ret = false;
    }

    switch (msg->what) {
        case GL_MSG_RENDERER:
            drawGL(glslFilter);

            if(times < 5){
                saveBmp();
            }
            times++;
            break;
        case GL_MSG_DECODED_FIRST_FRAME:

            break;
    }

    return ret;

}

bool GLThread::prepare() {

    initEGL();
    glslFilter = new GlslFilter();
    glslFilter->initial();
    return true;
}

void GLThread::SnapshotBmpRGB(GLvoid *pData, int width, int height, char *filename)
{

    BITMAPFILEHEADER fileHead={0};
    BITMAPINFOHEADER infoHead={0};
    int biBitCount = 24;
    long dataSize = (width*biBitCount+31)/32*4*height;
    FILE *fp =  fopen(filename, "wb");

    // 位图第一部分，文件信息
    fileHead.cfType[0] = 0x42;
    fileHead.cfType[1] = 0x4d;
    fileHead.cfSize = dataSize + 54;
    fileHead.cfReserved = 0;
    fileHead.cfoffBits = 54;
    // 位图第二部分，数据信息

    infoHead.ciSize[0] = 40;
    infoHead.ciWidth = width;
    infoHead.ciHeight = height;
    infoHead.ciPlanes[0] = 1;
    infoHead.ciBitCount = 24;
    infoHead.ciCompress[3] = 0;
    infoHead.ciSizeImage[3] = 0;
    infoHead.ciXPelsPerMeter[3] = 0;
    infoHead.ciYPelsPerMeter[3] = 0;
    infoHead.ciClrUsed[3] = 0;
    infoHead.ciClrImportant[3] = 0;

    fwrite(&fileHead,1, sizeof(BITMAPFILEHEADER),fp);
    fwrite(&infoHead,1, sizeof(BITMAPINFOHEADER),fp);
    fwrite(pData,1,dataSize,fp);
    fclose(fp);

}

void GLThread::SnapshotBmpRGBA(GLvoid * pData, int width, int height,  char * filename)
{

    BITMAPFILEHEADER fileHead={0};
    BITMAPINFOHEADER infoHead={0};
    int biBitCount = 32;
    long dataSize = (width*biBitCount+31)/32*4*height;
    FILE *fp =  fopen(filename, "wb");

    // 位图第一部分，文件信息
    fileHead.cfType[0] = 0x42;
    fileHead.cfType[1] = 0x4d;
    fileHead.cfSize = dataSize + 54;
    fileHead.cfReserved = 0;
    fileHead.cfoffBits = 54;
    // 位图第二部分，数据信息

    infoHead.ciSize[0] = 40;
    infoHead.ciWidth = width;
    infoHead.ciHeight = height;
    infoHead.ciPlanes[0] = 1;
    infoHead.ciBitCount = biBitCount;
    infoHead.ciCompress[3] = 0;
    infoHead.ciSizeImage[3] = 0;
    infoHead.ciXPelsPerMeter[3] = 0;
    infoHead.ciYPelsPerMeter[3] = 0;
    infoHead.ciClrUsed[3] = 0;
    infoHead.ciClrImportant[3] = 0;

    fwrite(&fileHead,1, sizeof(BITMAPFILEHEADER),fp);
    fwrite(&infoHead,1, sizeof(BITMAPINFOHEADER),fp);
    fwrite(pData,1,dataSize,fp);
    fclose(fp);

}

void GLThread::saveBmp(){
    int picture_size = mVideoStateInfo->mVideoWidth*mVideoStateInfo->mVideoHeight;
    unsigned char *RGBABuffer = (unsigned char *) malloc(picture_size * 4);
    unsigned char *BGRABuffer = (unsigned char *) malloc(picture_size * 4);
    unsigned char *RGBBuffer = (unsigned char *) malloc(picture_size * 3);
    unsigned char *BGRBuffer = (unsigned char *) malloc(picture_size * 3);

    memset(RGBABuffer,0,picture_size* 4);
    memset(RGBBuffer,0,picture_size* 3);

    memset(BGRABuffer,0,picture_size* 4);
    memset(BGRBuffer,0,picture_size* 3);

    const int BMP_ROW_ALIGN = 4;

    glPixelStorei(GL_PACK_ALIGNMENT, BMP_ROW_ALIGN);
    glReadPixels(0, 0, mVideoStateInfo->mVideoWidth, mVideoStateInfo->mVideoHeight,GL_RGBA,GL_UNSIGNED_BYTE,RGBABuffer);


    int strideRGBA=4;
    int strideBGR=3;
    // int len = mVideoWidth*mVideoHeight;

    for(int i=0;i<picture_size;i++){
        BGRBuffer[i*strideBGR]=RGBABuffer[i*strideRGBA+2]; //B
        BGRBuffer[i*strideBGR+1]=RGBABuffer[i*strideRGBA+1]; //R
        BGRBuffer[i*strideBGR+2]=RGBABuffer[i*strideRGBA]; //G

    }

    for(int i=0;i<picture_size;i++){
        BGRABuffer[i*strideRGBA]=RGBABuffer[i*strideRGBA+3];
        BGRABuffer[i*strideRGBA+1]=RGBABuffer[i*strideRGBA];
        BGRABuffer[i*strideRGBA+2]=RGBABuffer[i*strideRGBA+1];
        BGRABuffer[i*strideRGBA+3]=RGBABuffer[i*strideRGBA+2];
    }


    char file[1025]={0};
    sprintf(file,"/storage/emulated/0/egl%d.bmp",times);
    SnapshotBmpRGB(BGRBuffer, mVideoStateInfo->mVideoWidth, mVideoStateInfo->mVideoHeight, file);
    // SnapshotBmpRGBA(BGRABuffer, mVideoWidth, mVideoHeight, file);

//        image_t *imageFrame = gen_image(512,512);
//        imageFrame->buffer = RGBABuffer;
//        write_png("/storage/emulated/0/egl.png",imageFrame);
}

void GLThread::initEGL() {

    // EGL config attributes
    const EGLint confAttr[] =
            {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,// very important!
                    EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,//EGL_WINDOW_BIT EGL_PBUFFER_BIT we will create a pixelbuffer surface
                    EGL_RED_SIZE,   8,
                    EGL_GREEN_SIZE, 8,
                    EGL_BLUE_SIZE,  8,
                    EGL_ALPHA_SIZE, 8,// if you need the alpha channel
                    EGL_DEPTH_SIZE, 8,// if you need the depth buffer
                    EGL_STENCIL_SIZE,8,
                    EGL_NONE
            };
    // EGL context attributes
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,// very important!
            EGL_NONE
    };
    // surface attributes
    // the surface size is set to the input frame size
    const EGLint surfaceAttr[] = {
            EGL_WIDTH,512,
            EGL_HEIGHT,512,
            EGL_NONE
    };
    EGLint eglMajVers, eglMinVers;
    EGLint numConfigs;

    eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(eglDisp == EGL_NO_DISPLAY)
    {
        //Unable to open connection to local windowing system
        ALOGI("Unable to open connection to local windowing system");
    }
    if(!eglInitialize(eglDisp, &eglMajVers, &eglMinVers))
    {
        // Unable to initialize EGL. Handle and recover
        ALOGI("Unable to initialize EGL");
    }
    ALOGI("EGL init with version %d.%d", eglMajVers, eglMinVers);
    // choose the first config, i.e. best config
    if(!eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs))
    {
        ALOGI("some config is wrong");
    }
    else
    {
        ALOGI("all configs is OK");
    }
    // create a pixelbuffer surface
    eglSurface = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);
    //EGLNativeWindowType window = android_createDisplaySurface();
    //eglSurface = eglCreateWindowSurface(eglDisp, eglConf, NULL, surfaceAttr);
    if(eglSurface == EGL_NO_SURFACE)
    {
        switch(eglGetError())
        {
            case EGL_BAD_ALLOC:
                // Not enough resources available. Handle and recover
                ALOGI("Not enough resources available");
                break;
            case EGL_BAD_CONFIG:
                // Verify that provided EGLConfig is valid
                ALOGI("provided EGLConfig is invalid");
                break;
            case EGL_BAD_PARAMETER:
                // Verify that the EGL_WIDTH and EGL_HEIGHT are
                // non-negative values
                ALOGI("provided EGL_WIDTH and EGL_HEIGHT is invalid");
                break;
            case EGL_BAD_MATCH:
                // Check window and EGLConfig attributes to determine
                // compatibility and pbuffer-texture parameters
                ALOGI("Check window and EGLConfig attributes");
                break;
        }
    }
    eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);
    if(eglCtx == EGL_NO_CONTEXT)
    {
        EGLint error = eglGetError();
        if(error == EGL_BAD_CONFIG)
        {
            // Handle error and recover
            ALOGI("EGL_BAD_CONFIG");
        }
    }
    if(!eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx))
    {
        ALOGI("MakeCurrent failed");
    }
    ALOGI("initialize success!");

}

void GLThread::drawGL(GlslFilter *filter) {


    android_media_player_updateSurface(mVideoStateInfo->mTextureSurfaceObj);

    if(mPicture == NULL){
        mPicture = new Picture();
        mPicture->texture = filter->createTexture();
        mPicture->width = mVideoStateInfo->mVideoWidth;
        mPicture->height = mVideoStateInfo->mVideoHeight;
    }

    if(mTexturePicture == NULL){
        mTexturePicture = new Picture();
        mTexturePicture->texture = (GLuint) texture;
        mTexturePicture->width = mVideoStateInfo->mVideoWidth;
        mTexturePicture->height = mVideoStateInfo->mVideoHeight;
    }

    filter->process(mTexturePicture,NULL);
}

void GLThread::deInitEGL() {


    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;
}

void GLThread::stop() {

    int ret = -1;
    mVideoStateInfo->messageQueueGL->abort();
    mRunning = false;
    if ((ret = wait()) != 0) {
        ALOGE("Couldn't cancel GLThread: %i\n", ret);
        return;
    }


}