//
// Created by Administrator on 2016/12/1.
//

#ifndef YTXPLAYER_VIDEOSTATEINFO_H
#define YTXPLAYER_VIDEOSTATEINFO_H

#include "MessageQueue.h"
#include "jni.h"



class VideoStateInfo{
public:
    VideoStateInfo();
    ~VideoStateInfo();
    char* join(char *s1, char *s2);

public:

    int mVideoWidth;
    int mVideoHeight;
    MessageQueue *messageQueueGL;

    jobject mTextureSurfaceObj;
};
#endif //YTXPLAYER_VIDEOSTATEINFO_H
