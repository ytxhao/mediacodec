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
#include "GlslFilter.h"
#include "VideoStateInfo.h"
#include "GLThread.h"
#include "headerBmp.h"
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

    void rendererTexture();
    int isFinish;

    void decodeMovie(void* ptr);

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
    pthread_t					mPlayerPrepareAsyncThread;



    VideoStateInfo *mVideoStateInfo;


    GLThread *mGLThread;
};


#endif //YTXPLAYER_YTXMEDIAPLAYER_H
