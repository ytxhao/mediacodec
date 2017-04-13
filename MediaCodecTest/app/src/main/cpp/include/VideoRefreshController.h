//
// Created by Administrator on 2016/11/30.
//

#ifndef YTXPLAYER_VIDEOREFRESHTHREAD_H
#define YTXPLAYER_VIDEOREFRESHTHREAD_H
#include "frame_queue.h"
#include "Thread.h"
#include "VideoStateInfo.h"
#include "lock.h"
class VideoRefreshController : public Thread
{
public:
    double last_duration, duration, delay;
    Frame *vp, *lastvp;
    double remaining_time = 0.0;
    double time;
    double frame_timer=0.0;
    Lock mLock;
    VideoStateInfo* mVideoStateInfo;
    VideoRefreshController(VideoStateInfo* mVideoStateInfo);
    ~VideoRefreshController(){};

    void				handleRun(void* ptr);
    bool                prepare();
    void                refresh();
    void                process();
    void                stop();
    double              vpDuration(Frame *vp, Frame *next_vp);
    double              computeTargetDelay(double delay);
};


#endif //YTXPLAYER_VIDEOREFRESHTHREAD_H
