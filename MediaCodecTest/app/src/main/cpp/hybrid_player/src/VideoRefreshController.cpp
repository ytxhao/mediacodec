//
// Created by Administrator on 2016/11/30.
//
#define LOG_NDEBUG 0
#define TAG "YTX-VideoRefreshThread-JNI"

#include "ALog-priv.h"

#include <VideoStateInfo.h>
#include <android_media_YtxMediaPlayer.h>
#include <math.h>
#include "VideoRefreshController.h"
#include "ffinc.h"
#include "../../gl_engine/include/gl_engine.h"


VideoRefreshController::VideoRefreshController(VideoStateInfo *mVideoStateInfo) {

    last_duration = 0.0;
    duration = 0.0;
    delay = 0.0;
    vp = NULL;
    lastvp = NULL;
    remaining_time = 0.0;
    time = 0.0;
    frame_timer = 0.0;
    this->mVideoStateInfo = mVideoStateInfo;
}

void VideoRefreshController::handleRun(void *ptr) {
    if (!prepare()) {
        ALOGE("Couldn't prepare VideoRefreshController\n");
        return;
    }
    refresh();
}

bool VideoRefreshController::prepare() {
    return true;
}

#define REFRESH_RATE 0.01
#define AV_SYNC_THRESHOLD_MAX 0.1

void VideoRefreshController::process() {
    if (mVideoStateInfo != NULL) {

        if (remaining_time > 0.0) {
            av_usleep((unsigned int) (remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;

        if (mVideoStateInfo->frameQueueVideo->frameQueueNumRemaining() < 1) {
            // nothing to do, no picture to display in the queue

        } else {


            lastvp = mVideoStateInfo->frameQueueVideo->frameQueuePeekLast();
            vp = mVideoStateInfo->frameQueueVideo->frameQueuePeek();

/*
            last_duration = vpDuration(lastvp, vp);

            delay = last_duration;



            time = av_gettime_relative() / 1000000.0; //获取ff系统时间,单位为秒

            if (time < frame_timer + delay) { //如果当前时间小于(frame_timer+delay)则不去frameQueue取下一帧直接刷新当前帧
                remaining_time = FFMIN(frame_timer + delay - time, remaining_time); //显示下一帧还差多长时间
                return;
            }

            frame_timer += delay; //下一帧需要在这个时间显示
            if (delay > 0 && time - frame_timer > AV_SYNC_THRESHOLD_MAX) {
                frame_timer = time;
            }
*/
            if (lastvp->pixels != NULL ) {

                addRendererVideoFrameRGBA(mVideoStateInfo->GraphicRendererObj,
                                          lastvp->pixels,
                                          mVideoStateInfo->mVideoWidth,
                                          mVideoStateInfo->mVideoHeight);

                android_media_player_notifyRenderFrame(mVideoStateInfo->VideoGlSurfaceViewObj);

            }
            mVideoStateInfo->frameQueueVideo->frameQueueNext();
        }
    }

}


void VideoRefreshController::stop() {
    mRunning = false;
    int ret = -1;
    if ((ret = wait()) != 0) {
        ALOGE("Couldn't cancel IDecoder: %i\n", ret);
        return;
    }
}

void VideoRefreshController::refresh() {

    while (mRunning) {
        process();
    }
    //结束视频刷新

    android_media_player_notifyRenderFrame(mVideoStateInfo->VideoGlSurfaceViewObj);

}

double VideoRefreshController::vpDuration(Frame *vp, Frame *next_vp) {
    if (vp->serial == next_vp->serial) {
        double duration = next_vp->pts - vp->pts;
        if (isnan(duration) || duration <= 0)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}
