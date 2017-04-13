//
// Created by Administrator on 2016/12/1.
//

#ifndef YTXPLAYER_VIDEOSTATEINFO_H
#define YTXPLAYER_VIDEOSTATEINFO_H

#include "MessageQueue.h"
#include "frame_queue_video.h"
#include "jni.h"
#include "InputStream.h"



class VideoStateInfo{
public:
    VideoStateInfo();
    ~VideoStateInfo();
    char* join(char *s1, char *s2);

public:

    int mVideoWidth;
    int mVideoHeight;
    MessageQueue *messageQueueGL;
    FrameQueue *frameQueueVideo;
    Frame *vp;

    jobject mTextureSurfaceObj;
    jobject VideoGlSurfaceViewObj;
    jobject GraphicRendererObj;

    int st_index[AVMEDIA_TYPE_NB];
    AVFormatContext *pFormatCtx;
    double max_frame_duration;

    InputStream *streamVideo;
    InputStream *streamAudio;
    InputStream *streamSubtitle;



    enum AVSampleFormat in_sample_fmt ;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate ;
    //输出采样率
    int out_sample_rate = 44100;
    int out_channel_nb;
    int out_nb_samples;
    uint64_t  in_ch_layout;
    uint64_t out_ch_layout;
};
#endif //YTXPLAYER_VIDEOSTATEINFO_H
