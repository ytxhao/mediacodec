//
// Created by Administrator on 2016/12/1.
//
#define NDEBUG 0
#define TAG "VideoStateInfo-jni"


#include <android_media_YtxMediaPlayer.h>
#include <stdlib.h>
#include "VideoStateInfo.h"
#include "ALog-priv.h"

VideoStateInfo::VideoStateInfo() {

    messageQueueGL = new MessageQueue();
    mVideoWidth = 0;
    mVideoHeight = 0;



}

VideoStateInfo::~VideoStateInfo() {


}



char *VideoStateInfo::join(char *s1, char *s2) {
    char *result = (char *) malloc(strlen(s1) + strlen(s2) + 1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    if (result == NULL) exit(1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}
