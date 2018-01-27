//
// Created by Administrator on 2016/9/2.
//

#define LOG_NDEBUG 0
#define TAG "YTX-PLAYER-JNI"


#include <HybridMediaPlayer.h>

#include "../../include/ALog-priv.h"

//该文件必须包含在源文件中(*.cpp),以免宏展开时提示重复定义的错误



HybridMediaPlayer::HybridMediaPlayer() {

    wanted_stream_spec[AVMEDIA_TYPE_VIDEO] = "vst";
    wanted_stream_spec[AVMEDIA_TYPE_AUDIO] = "ast";
    wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] = "sst";

    mVideoStateInfo = new VideoStateInfo();
    mGLThread = new GLThread(mVideoStateInfo);
    mVideoRefreshController = new VideoRefreshController(mVideoStateInfo);
    mVideoStateInfo->frameQueueVideo->frameQueueInit(VIDEO_PICTURE_QUEUE_SIZE, 1);
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
    mGLThread->texture = texture;
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
//            AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_WIDTH,640);
//            AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_HEIGHT,160);
            ALOGI("track mime: %s", mime);
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


    pthread_create(&mPlayerPrepareAsyncThread, NULL, prepareAsyncPlayer, this);

    return 0;
}

void *HybridMediaPlayer::prepareAsyncPlayer(void *ptr) {

    HybridMediaPlayer* mPlayer = (HybridMediaPlayer *) ptr;

    av_register_all();
    avformat_network_init();
    mPlayer->pFormatCtx = avformat_alloc_context();
    ALOGI("prepareAsyncPlayer prepare this->filePath=%s\n",mPlayer->filePath);
    //   ALOGI("Couldn't open input stream.\n");
    if(avformat_open_input(&mPlayer->pFormatCtx,mPlayer->filePath,NULL,NULL)!=0){
        ALOGI("Couldn't open input stream.\n");
        return 0;
    }

    if(avformat_find_stream_info(mPlayer->pFormatCtx,NULL)<0){
        ALOGI("Couldn't find stream information.\n");
        return 0;
    }


    for(int i=0; i<mPlayer->pFormatCtx->nb_streams; i++) {
        AVStream *st = mPlayer->pFormatCtx->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;

        if (type >= 0 && mPlayer->wanted_stream_spec[type] && mPlayer->mVideoStateInfo->st_index[type] == -1) {
            if (avformat_match_stream_specifier(mPlayer->pFormatCtx, st, mPlayer->wanted_stream_spec[type]) > 0) {
                mPlayer->mVideoStateInfo->st_index[type] = i;
            }
        }

    }

    for (int i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (mPlayer->wanted_stream_spec[i] && mPlayer->mVideoStateInfo->st_index[i] == -1) {
            ALOGI("Stream specifier %s does not match any %s stream\n", mPlayer->wanted_stream_spec[(AVMediaType)i], av_get_media_type_string((AVMediaType)i));
            mPlayer->mVideoStateInfo->st_index[i] = INT_MAX;
        }
    }


    mPlayer->mVideoStateInfo->pFormatCtx = mPlayer->pFormatCtx;
    mPlayer->mVideoStateInfo->max_frame_duration = (mPlayer->mVideoStateInfo->pFormatCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    ALOGI("mVideoStateInfo->max_frame_duration=%lf\n",mPlayer->mVideoStateInfo->max_frame_duration);
    if(mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_AUDIO] >= 0){
        mPlayer->streamComponentOpen(mPlayer->mVideoStateInfo->streamAudio,mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_AUDIO]);
    }

    if(mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_VIDEO] >= 0){
        mPlayer->streamComponentOpen(mPlayer->mVideoStateInfo->streamVideo,mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_VIDEO]);
    }

    if(mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_SUBTITLE] >= 0){
        mPlayer->streamComponentOpen(mPlayer->mVideoStateInfo->streamSubtitle,mPlayer->mVideoStateInfo->st_index[AVMEDIA_TYPE_SUBTITLE]);
    }



    mPlayer->mListener->notify(1, 0, 0);
    return 0;
}


int HybridMediaPlayer::resume() {

    return 0;
}

int HybridMediaPlayer::start() {

    ALOGI("HybridMediaPlayer start\n");
   // pthread_create(&mGLThread, NULL, startGLThread, this);



    pthread_create(&mPlayerThread, NULL, startPlayer, this);
    mGLThread->startAsync();
    return 0;
}

void *HybridMediaPlayer::startPlayer(void *ptr) {

    ALOGI("starting main player thread\n");
    HybridMediaPlayer *mPlayer = (HybridMediaPlayer *) ptr;
    mPlayer->decodeMovie(ptr);

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
//        AMediaFormat* mAMediaFormat =  AMediaCodec_getOutputFormat(d->codec);
//        AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_WIDTH,&mVideoStateInfo->mVideoWidth);
//        AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_HEIGHT,&mVideoStateInfo->mVideoHeight);
//        ALOGI("msg.what = GL_MSG_DECODED_FIRST_FRAME for test mVideoWidth=%d,mVideoHeight=%d",mVideoStateInfo->mVideoWidth,mVideoStateInfo->mVideoHeight);
        //AMediaFormat* mAMediaFormat =  AMediaCodec_getOutputFormat(d->codec);
        //AMediaFormat_setInt32(mAMediaFormat,AMEDIAFORMAT_KEY_HEIGHT,200);
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

            if(mVideoStateInfo->mVideoWidth == 0 || mVideoStateInfo->mVideoHeight == 0){
                AMediaFormat* mAMediaFormat =  AMediaCodec_getOutputFormat(d->codec);
                AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_WIDTH,&mVideoStateInfo->mVideoWidth);
                AMediaFormat_getInt32(mAMediaFormat,AMEDIAFORMAT_KEY_HEIGHT,&mVideoStateInfo->mVideoHeight);
                //mVideoStateInfo->mVideoHeight = 360;
                AVMessage msg;
                msg.what = GL_MSG_DECODED_FIRST_FRAME;
                mVideoStateInfo->messageQueueGL->put(&msg);
                ALOGI("msg.what = GL_MSG_DECODED_FIRST_FRAME for test mVideoWidth=%d,mVideoHeight=%d",mVideoStateInfo->mVideoWidth,mVideoStateInfo->mVideoHeight);
            }

            int64_t presentationNano = info.presentationTimeUs * 1000;
            ALOGI("info.presentationTimeUs=%lld presentationNano=%lld",info.presentationTimeUs,presentationNano);
            if (d->renderstart < 0) {
                d->renderstart = systemNanoTime() - presentationNano;
            }
            int64_t delay = (d->renderstart + presentationNano) - systemNanoTime();
            if (delay > 0) {
                usleep(delay / 1000);
            }

            mVideoStateInfo->vp = mVideoStateInfo->frameQueueVideo->frameQueuePeekWritable();
            mVideoStateInfo->vp->pts = info.presentationTimeUs;

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

    mVideoRefreshController->startAsync();

    while(!data.sawInputEOS || !data.sawOutputEOS){
        doCodecWork(&data);
    }

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

void HybridMediaPlayer::rendererTexture(){

    ALOGI("HybridMediaPlayer::rendererTexture");
    AVMessage msg;
    msg.what = GL_MSG_RENDERER;
    mVideoStateInfo->messageQueueGL->put(&msg);


}


int HybridMediaPlayer::streamComponentOpen(InputStream *is, int stream_index)
{

    if (stream_index < 0 || stream_index > pFormatCtx->nb_streams) {
        return -1;
    }


    is->dec_ctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(is->dec_ctx, pFormatCtx->streams[stream_index]->codecpar);

    av_codec_set_pkt_timebase(is->dec_ctx, pFormatCtx->streams[stream_index]->time_base);
    is->st = pFormatCtx->streams[stream_index];
    AVCodec* codec = avcodec_find_decoder(is->dec_ctx->codec_id);
    if (codec == NULL) {
        return -1;
    }

    ALOGI("streamVideo.dec_ctx->codec_id=%d\n",is->dec_ctx->codec_id);

    is->dec_ctx->codec_id = codec->id;



    // Open codec
    if (avcodec_open2(is->dec_ctx, codec,NULL) < 0) {
        return -1;
    }


    switch (is->dec_ctx->codec_type){
        case AVMEDIA_TYPE_AUDIO:
        {
            //解压缩数据

            //frame->16bit 44100 PCM 统一音频采样格式与采样率
            //重采样设置参数-------------start
            //输入的采样格式
            mVideoStateInfo->in_sample_fmt = is->dec_ctx->sample_fmt;
            //输出采样格式16bit PCM
            mVideoStateInfo->out_sample_fmt = AV_SAMPLE_FMT_S16;
            //输入采样率
            mVideoStateInfo->in_sample_rate = is->dec_ctx->sample_rate;
            //输出采样率
            //out_sample_rate = 44100;
            mVideoStateInfo->out_sample_rate = mVideoStateInfo->in_sample_rate;
            //获取输入的声道布局
            //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
            //av_get_default_channel_layout(codecCtx->channels);
            mVideoStateInfo->out_nb_samples=is->dec_ctx->frame_size;

            mVideoStateInfo->in_ch_layout = is->dec_ctx->channel_layout;
            //输出的声道布局（立体声）
            mVideoStateInfo->out_ch_layout = AV_CH_LAYOUT_STEREO;

            //输出的声道个数
            mVideoStateInfo->out_channel_nb = av_get_channel_layout_nb_channels(mVideoStateInfo->out_ch_layout);

            //重采样设置参数-------------end

            ALOGI("### in_sample_rate=%d\n",mVideoStateInfo->in_sample_rate);
            ALOGI("### in_sample_fmt=%d\n",mVideoStateInfo->in_sample_fmt);
            ALOGI("### out_nb_samples=%d\n",mVideoStateInfo->out_nb_samples);
            ALOGI("### out_sample_rate=%d\n",mVideoStateInfo->out_sample_rate);
            ALOGI("### out_sample_fmt=%d\n",mVideoStateInfo->out_sample_fmt);
            ALOGI("### out_channel_nb=%d\n",mVideoStateInfo->out_channel_nb);

            ALOGI("### in_ch_layout=%d\n",mVideoStateInfo->in_ch_layout);
            ALOGI("### out_ch_layout=%d\n",mVideoStateInfo->out_ch_layout);
        }
            break;
        case AVMEDIA_TYPE_VIDEO:

//            mVideoStateInfo->mVideoWidth = is->dec_ctx->width;
//            mVideoStateInfo->mVideoHeight = is->dec_ctx->height;
            mDuration =  pFormatCtx->duration;

            break;
        case AVMEDIA_TYPE_SUBTITLE:

            ALOGI("AVMEDIA_TYPE_SUBTITLE");

            break;
    }


    return 0;
}