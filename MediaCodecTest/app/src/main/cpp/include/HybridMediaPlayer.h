//
// Created by Administrator on 2016/9/2.
//

#ifndef YTXPLAYER_YTXMEDIAPLAYER_H
#define YTXPLAYER_YTXMEDIAPLAYER_H


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
    int fd;
    ANativeWindow* window;
    AMediaExtractor* ex;
    AMediaCodec *codec;
    int64_t renderstart;
    bool sawInputEOS;
    bool sawOutputEOS;
    bool isPlaying;
    bool renderonce;
} workerdata;


typedef struct image_s {
    int width, height, stride;
    unsigned char *buffer;      // RGB24
} image_t;



//14byte文件头
typedef struct
{
    char cfType[2];//文件类型，"BM"(0x4D42)
    long cfSize;//文件大小（字节）
    long cfReserved;//保留，值为0
    long cfoffBits;//数据区相对于文件头的偏移量（字节）
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

//40byte信息头
typedef struct
{
    char ciSize[4];//BITMAPFILEHEADER所占的字节数
    long ciWidth;//宽度
    long ciHeight;//高度
    char ciPlanes[2];//目标设备的位平面数，值为1
    int ciBitCount;//每个像素的位数
    char ciCompress[4];//压缩说明
    char ciSizeImage[4];//用字节表示的图像大小，该数据必须是4的倍数
    char ciXPelsPerMeter[4];//目标设备的水平像素数/米
    char ciYPelsPerMeter[4];//目标设备的垂直像素数/米
    char ciClrUsed[4]; //位图使用调色板的颜色数
    char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct
{
    unsigned short blue;
    unsigned short green;
    unsigned short red;
    unsigned short reserved;
}__attribute__((packed)) PIXEL;//颜色模式RGB

class HybridMediaPlayer {


public:


    HybridMediaPlayer();

    ~HybridMediaPlayer();

    void died();

    void disconnect();

    int setSubtitles( char *url);
    int setDataSource(const char *url);
    long long getFileSize(int fd);

    void setTexture(int texture);

    int setDataSource(int fd, int64_t offset, int64_t length);

    int setListener(MediaPlayerListener* listener);
    int prepare();

    int prepareAsync();


    int start();

    static void* startPlayer(void* ptr);
    static void* startGLThread(void* ptr);
    static void* prepareAsyncPlayer(void* ptr);

    void write_png(char *fname, image_t *img);
    image_t *gen_image(int width, int height);
    void packetEnoughWait();
    int stop();

    bool isRelease;
    int release();

    int pause();

    int resume();
    bool isPlaying();

    int getVideoWidth();

    int getVideoHeight();

    void checkSeekRequest();
    int64_t systemNanoTime();
    void doCodecWork(workerdata *d);
    int seekTo(int msec);

    int getCurrentPosition();

    int getDuration();

    int reset();


    int setLooping(int loop);

    bool isLooping();

    int setVolume(float leftVolume, float rightVolume);


    int setAudioSessionId(int sessionId);

    int getAudioSessionId();

    int setAuxEffectSendLevel(float level);

    int attachAuxEffect(int effectId);


    int setRetransmitEndpoint(const char *addrString, uint16_t port);

    int updateProxyConfig(
            const char *host, int32_t port, const char *exclusionList);


    void notifyRenderer();

    void finish();

    int isFinish;

    void decodeMovie(void* ptr);
    void runGLThread(void* ptr);
    void initEGL();
    void deInitEGL();
    void drawGL();
    bool getExitPendingGL();
    void setExitPendingGL(bool exitPending);
    bool mExitPending;
    void clear_l();

    int seekTo_l(int msec);

    int prepareAsync_l();

    int getDuration_l(int *msec);

    int reset_l();

    HybridMediaPlayer *mPlayer;

    MediaPlayerListener     *mListener;
    void *mCookie;

    int mCurrentPosition;
    int mSeekPosition;
    bool mPrepareSync;
    int mPrepareStatus;

    bool mLoop;
    float mLeftVolume;
    float mRightVolume;

    int mAudioSessionId;
    float mSendLevel;

    bool mRetransmitEndpointValid;
    const char *filePath;
    char *subtitles=NULL;
    //AVFormatContext *pFormatCtx;
    int mAudioStreamIndex;
    int mVideoStreamIndex;

    int  mDuration;

    int abortRequest;


    int mStreamType;
    int mCurrentState;

    int  got_picture;

    workerdata data = {-1, NULL, NULL, NULL, 0, false, false, false, false};
    pthread_t					mPlayerThread;
    pthread_t					mGLThread;
    pthread_t					mPlayerPrepareAsyncThread;


     EGLConfig eglConf;
     EGLSurface eglSurface;
     EGLContext eglCtx;
     EGLDisplay eglDisp;

    int texture;
};


#endif //YTXPLAYER_YTXMEDIAPLAYER_H
