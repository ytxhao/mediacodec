//
// Created by Administrator on 2016/10/13.
//

#include "frame_queue_video.h"

#define TAG "YTX-FrameQueueVideo-JNI"


FrameQueueVideo::~FrameQueueVideo() {
    frameQueueDestroy();
}

void FrameQueueVideo::frameQueueUnrefItem(Frame *vp) {

    if(vp->pixels != NULL ){
        free(vp->pixels);
        vp->pixels = NULL;
    }
}


int FrameQueueVideo::frameQueueInit(int max_size, int keep_last) {
    int i = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    this->max_size = max_size;
    this->keep_last = keep_last;
    for (i = 0; i < max_size; i++) {
        queue[i].pixels = NULL;

    }
    return 1;
}

void FrameQueueVideo::frameQueueDestroy() {
    int i;
    for (i = 0; i < max_size; i++) {
        Frame *vp = &queue[i];
        frameQueueUnrefItem(vp);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}