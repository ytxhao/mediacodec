//
// Created by Administrator on 2016/9/2.
//

#define LOG_NDEBUG 0
#define TAG "YTX-PLAYER-JNI"


#include <HybridMediaPlayer.h>
#include <GlslFilter.h>
#include <android_media_YtxMediaPlayer.h>
#include "../../include/ALog-priv.h"

//该文件必须包含在源文件中(*.cpp),以免宏展开时提示重复定义的错误
#include "../../include/HybridMediaPlayer.h"
#include "../../gl_engine/include/gl_engine.h"



HybridMediaPlayer::HybridMediaPlayer() {
    mExitPending = false;
    mPicture = NULL;
    mTexturePicture = NULL;
}

HybridMediaPlayer::~HybridMediaPlayer() {

}

void HybridMediaPlayer::died() {

}


void HybridMediaPlayer::disconnect() {

}

long long HybridMediaPlayer::getFileSize(int fd)
{
    struct stat64 buf;
    if(fstat64(fd, &buf)<0)
    {
        return 0;
    }
    return buf.st_size;
}

void HybridMediaPlayer::setTexture(int texture){
    this->texture = texture;

}
int HybridMediaPlayer::setDataSource(const char *url) {

    long long outStart, outLen;
    this->filePath = url;
    ALOGI("HybridMediaPlayer setDataSource filePath=%s\n", filePath);
    data.fd = open(url, O_RDONLY);
    outLen = getFileSize(data.fd);
    AMediaExtractor *ex = AMediaExtractor_new();
    media_status_t err = AMediaExtractor_setDataSourceFd(ex, data.fd,
                                                         0,
                                                         static_cast<off64_t>(outLen));
    close(data.fd);
    if (err != AMEDIA_OK) {
        ALOGE("setDataSource error: %d", err);
        return JNI_FALSE;
    }
    int numtracks = AMediaExtractor_getTrackCount(ex);
    AMediaCodec *codec = NULL;
    ALOGI("input has %d tracks", numtracks);
    for (int i = 0; i < numtracks; i++) {
        AMediaFormat *format = AMediaExtractor_getTrackFormat(ex, i);
        const char *s = AMediaFormat_toString(format);
        ALOGI("track %d format: %s", i, s);
        const char *mime;
        if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
            ALOGI("no mime type");
            return JNI_FALSE;
        } else if (!strncmp(mime, "video/", 6)) {
            // Omitting most error handling for clarity.
            // Production code should check for errors.
            AMediaExtractor_selectTrack(ex, i);
            codec = AMediaCodec_createDecoderByType(mime);
            AMediaCodec_configure(codec, format, data.window, NULL, 0);
            data.ex = ex;
            data.codec = codec;
            data.renderstart = -1;
            data.sawInputEOS = false;
            data.sawOutputEOS = false;
            data.isPlaying = false;
            data.renderonce = true;
            AMediaCodec_start(codec);
        }
        AMediaFormat_delete(format);
    }


    return 0;
}

int HybridMediaPlayer::setSubtitles(char *url) {

    this->subtitles = url;
    ALOGI("HybridMediaPlayer setSubtitles subtitles=%s\n", subtitles);
    return 0;
}


int HybridMediaPlayer::setDataSource(int fd, int64_t offset, int64_t length) {

    return 0;
}


int HybridMediaPlayer::prepare() {

    return 0;

}


int HybridMediaPlayer::prepareAsync() {

    mListener->notify(1, 0, 0);
    return 0;
}

void *HybridMediaPlayer::prepareAsyncPlayer(void *ptr) {

}


int HybridMediaPlayer::resume() {

    return 0;
}

int HybridMediaPlayer::start() {

    ALOGI("HybridMediaPlayer start\n");
    pthread_create(&mGLThread, NULL, startGLThread, this);
    pthread_create(&mPlayerThread, NULL, startPlayer, this);
    return 0;
}

void *HybridMediaPlayer::startPlayer(void *ptr) {

    ALOGI("starting main player thread\n");
    HybridMediaPlayer *mPlayer = (HybridMediaPlayer *) ptr;
    mPlayer->decodeMovie(ptr);

    return 0;
}

void *HybridMediaPlayer::startGLThread(void *ptr) {

    sleep(1);
    HybridMediaPlayer *mPlayer = (HybridMediaPlayer *) ptr;

    mPlayer->runGLThread(ptr);
    return 0;
}
void HybridMediaPlayer::checkSeekRequest() {

}

int64_t HybridMediaPlayer::systemNanoTime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
}


void HybridMediaPlayer::doCodecWork(workerdata *d) {

    ssize_t bufidx = -1;
    if (!d->sawInputEOS) {
        bufidx = AMediaCodec_dequeueInputBuffer(d->codec, 2000);
        //ALOGI("input buffer %zd", bufidx);
        if (bufidx >= 0) {
            size_t bufsize;
            auto buf = AMediaCodec_getInputBuffer(d->codec, bufidx, &bufsize);
            auto sampleSize = AMediaExtractor_readSampleData(d->ex, buf, bufsize);
            if (sampleSize < 0) {
                sampleSize = 0;
                d->sawInputEOS = true;
                ALOGI("EOS");
            }
            auto presentationTimeUs = AMediaExtractor_getSampleTime(d->ex);

            AMediaCodec_queueInputBuffer(d->codec, bufidx, 0, sampleSize, presentationTimeUs,
                                         d->sawInputEOS ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
            AMediaExtractor_advance(d->ex);
        }
    }

    if (!d->sawOutputEOS) {
        AMediaCodecBufferInfo info;
        auto status = AMediaCodec_dequeueOutputBuffer(d->codec, &info, 0);
        if (status >= 0) {
            //ALOGI("output buffers ok");
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                ALOGI("output EOS");
                d->sawOutputEOS = true;
            }

            AMediaFormat* mAMediaFormat =  AMediaCodec_getOutputFormat(d->codec);
            AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_WIDTH,&mVideoWidth);
            AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_HEIGHT,&mVideoHeight);
            int64_t presentationNano = info.presentationTimeUs * 1000;
            if (d->renderstart < 0) {
                d->renderstart = systemNanoTime() - presentationNano;
            }
            int64_t delay = (d->renderstart + presentationNano) - systemNanoTime();
            if (delay > 0) {
                usleep(delay / 1000);
            }

            AMediaCodec_releaseOutputBuffer(d->codec, status, info.size != 0);
            if (d->renderonce) {
                d->renderonce = false;
                return;
            }
        } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            ALOGI("output buffers changed");
        } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            auto format = AMediaCodec_getOutputFormat(d->codec);
            ALOGI("format changed to: %s", AMediaFormat_toString(format));
            AMediaFormat_delete(format);
        } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            ALOGI("no output buffer right now");
        } else {
            ALOGI("unexpected info code: %zd", status);
        }
    }

//    if (!d->sawInputEOS || !d->sawOutputEOS) {
//        mlooper->post(kMsgCodecBuffer, d);
//    }
}

void HybridMediaPlayer::decodeMovie(void *ptr) {

    while(!data.sawInputEOS || !data.sawOutputEOS){
        doCodecWork(&data);
    }

}

void HybridMediaPlayer::drawGL(GlslFilter *filter) {

    android_media_player_updateSurface(mTextureSurfaceObj);

    if(mPicture == NULL){
        mPicture = new Picture();
        mPicture->texture = filter->createTexture();
        mPicture->width = mVideoWidth;
        mPicture->height = mVideoHeight;
    }

    if(mTexturePicture == NULL){
        mTexturePicture = new Picture();
        mTexturePicture->texture = (GLuint) texture;
        mTexturePicture->width = mVideoWidth;
        mTexturePicture->height = mVideoHeight;
    }

    filter->process(mTexturePicture,NULL);

}

void HybridMediaPlayer::drawGL1() {

    const char vertex_shader_fix[]=
            "attribute vec4 a_Position;\n"
                    "void main() {\n"
                    "	gl_Position=a_Position;\n"
                    "}\n";

    const char fragment_shader_simple[]=
            "precision mediump float;\n"
                    "void main(){\n"
                    "	gl_FragColor = vec4(0.0,1.0,0.0,1.0);\n"
                    "}\n";

    const float tableVerticesWithTriangles[] = {
            // Triangle1
            -0.5f,-0.5f,
            0.5f, 0.5f,
            -0.5f, 0.5f,
            // Triangle2
            -0.5f,-0.5f,
            0.5f,-0.5f,
            0.5f, 0.5f,
    };

    const char*vertex_shader=vertex_shader_fix;
    const char*fragment_shader=fragment_shader_simple;
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glClearColor(0.0,0.0,0.0,0.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glViewport(0,0,512,512);
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertex_shader,NULL);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragment_shader,NULL);
    glCompileShader(fragmentShader);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);
    GLuint aPositionLocation =glGetAttribLocation(program, "a_Position");
    glVertexAttribPointer(aPositionLocation,2,GL_FLOAT,GL_FALSE,0,tableVerticesWithTriangles);
    glEnableVertexAttribArray(aPositionLocation);
    //draw something
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES,0,6);
    eglSwapBuffers(eglDisp,eglSurface);

}

bool HybridMediaPlayer::getExitPendingGL() {
    return mExitPending;
}

void HybridMediaPlayer::setExitPendingGL(bool exitPending) {
    mExitPending = exitPending;
}

void SnapshotBmpRGB(GLvoid *pData, int width, int height, char *filename)
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

void SnapshotBmpRGBA(GLvoid * pData, int width, int height,  char * filename)
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

void HybridMediaPlayer::runGLThread(void *ptr) {
    ALOGI("runGLThread mVideoWidth=%d mVideoHeight=%d",mVideoWidth,mVideoHeight);
    unsigned char *RGBABuffer = (unsigned char *) malloc(mVideoWidth * mVideoHeight * 4);
    unsigned char *BGRABuffer = (unsigned char *) malloc(mVideoWidth * mVideoHeight * 4);
    unsigned char *RGBBuffer = (unsigned char *) malloc(mVideoWidth * mVideoHeight * 3);
    unsigned char *BGRBuffer = (unsigned char *) malloc(mVideoWidth * mVideoHeight * 3);

    memset(RGBABuffer,0,mVideoWidth * mVideoHeight* 4);
    memset(RGBBuffer,0,mVideoWidth * mVideoHeight* 3);

    memset(BGRABuffer,0,mVideoWidth * mVideoHeight* 4);
    memset(BGRBuffer,0,mVideoWidth * mVideoHeight* 3);

    const int BMP_ROW_ALIGN = 4;


    initEGL();

    glslFilter = new GlslFilter();
    glslFilter->initial();

    int times = 0;
    while (!getExitPendingGL()){
        times++;
        glslFilter->renderBackground();

        //drawGL1();

        drawGL(glslFilter);

        glPixelStorei(GL_PACK_ALIGNMENT, BMP_ROW_ALIGN);
        glReadPixels(0, 0, mVideoWidth, mVideoHeight,GL_RGBA,GL_UNSIGNED_BYTE,RGBABuffer);


        int strideRGBA=4;
        int strideBGR=3;
        int len = mVideoWidth*mVideoHeight;

        for(int i=0;i<len;i++){
            BGRBuffer[i*strideBGR]=RGBABuffer[i*strideRGBA+2]; //B
            BGRBuffer[i*strideBGR+1]=RGBABuffer[i*strideRGBA+1]; //R
            BGRBuffer[i*strideBGR+2]=RGBABuffer[i*strideRGBA]; //G

        }

        for(int i=0;i<len;i++){
            BGRABuffer[i*strideRGBA]=RGBABuffer[i*strideRGBA+3];
            BGRABuffer[i*strideRGBA+1]=RGBABuffer[i*strideRGBA];
            BGRABuffer[i*strideRGBA+2]=RGBABuffer[i*strideRGBA+1];
            BGRABuffer[i*strideRGBA+3]=RGBABuffer[i*strideRGBA+2];
        }


        char file[1025]={0};
        sprintf(file,"/storage/emulated/0/egl%d.bmp",times);
        SnapshotBmpRGB(BGRBuffer, mVideoWidth, mVideoHeight, file);
       // SnapshotBmpRGBA(BGRABuffer, mVideoWidth, mVideoHeight, file);

//        image_t *imageFrame = gen_image(512,512);
//        imageFrame->buffer = RGBABuffer;
//        write_png("/storage/emulated/0/egl.png",imageFrame);
        if(times == 5){
            setExitPendingGL(true);
        }

    }

    deInitEGL();

    ALOGI("runGLThread END");
}


void HybridMediaPlayer::initEGL() {

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

image_t *HybridMediaPlayer::gen_image(int width, int height) {

    image_t *img = (image_t *) malloc(sizeof(image_t));
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->buffer = (unsigned char *) calloc(1, height * width * 4);
    memset(img->buffer, 0, img->stride * img->height);

    return img;
}

void HybridMediaPlayer::write_png(char *fname, image_t *img)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **row_pointers;
    int k;

    png_ptr =
            png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    fp = NULL;

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    fp = fopen(fname, "wb");
    if (fp == NULL) {
        ALOGI("PNG Error opening %s for writing!\n", fname);
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_compression_level(png_ptr, 0);

    png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_set_bgr(png_ptr);

    row_pointers = (png_byte **) malloc(img->height * sizeof(png_byte *));
    for (k = 0; k < img->height; k++)
        row_pointers[k] = img->buffer + img->stride * k;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_pointers);

    fclose(fp);
}

void HybridMediaPlayer::deInitEGL() {

    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;

}
void HybridMediaPlayer::packetEnoughWait() {


}

int HybridMediaPlayer::release() {
    ALOGI("HybridMediaPlayer::release");
    isRelease = true;
    return 0;
}

int HybridMediaPlayer::stop() {

    return 0;
}

int HybridMediaPlayer::pause() {

    return 0;
}

bool HybridMediaPlayer::isPlaying() {

    return 0;
}

int HybridMediaPlayer::getVideoWidth() {

    return 0;
}

int HybridMediaPlayer::getVideoHeight() {

    return 0;
}

int HybridMediaPlayer::seekTo(int msec) {

    return 0;
}

int HybridMediaPlayer::getCurrentPosition() {
    return 0;
}

int HybridMediaPlayer::getDuration() {

    return 0;
}

int HybridMediaPlayer::reset() {

    return 0;
}


int HybridMediaPlayer::setLooping(int loop) {

    return 0;
}

bool HybridMediaPlayer::isLooping() {

    return 0;
}

int HybridMediaPlayer::setVolume(float leftVolume, float rightVolume) {

    return 0;
}

int HybridMediaPlayer::setAudioSessionId(int sessionId) {

    return 0;
}

int HybridMediaPlayer::getAudioSessionId() {

    return 0;
}

int HybridMediaPlayer::setAuxEffectSendLevel(float level) {

    return 0;
}

int HybridMediaPlayer::attachAuxEffect(int effectId) {

    return 0;
}

int HybridMediaPlayer::setRetransmitEndpoint(const char *addrString, uint16_t port) {

    return 0;
}


int HybridMediaPlayer::updateProxyConfig(const char *host, int32_t port, const char *exclusionList) {

    return 0;
}

void HybridMediaPlayer::clear_l() {


}

int HybridMediaPlayer::seekTo_l(int msec) {

    return 0;
}

int HybridMediaPlayer::prepareAsync_l() {

    return 0;
}

int HybridMediaPlayer::getDuration_l(int *msec) {

    return 0;
}

int HybridMediaPlayer::reset_l() {

    return 0;
}

int HybridMediaPlayer::setListener(MediaPlayerListener *listener) {

    mListener = listener;

    return 0;
}

void HybridMediaPlayer::finish() {

    ALOGI("HybridMediaPlayer::finish IN");
    // mVideoStateInfo->mMessageLoop->stop();
    isFinish = 1;
    ALOGI("HybridMediaPlayer::finish OUT");
}
