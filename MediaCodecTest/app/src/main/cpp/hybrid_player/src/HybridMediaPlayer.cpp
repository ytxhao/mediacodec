//
// Created by Administrator on 2016/9/2.
//

#define LOG_NDEBUG 0
#define TAG "YTX-PLAYER-JNI"


#include <HybridMediaPlayer.h>
#include "../../include/ALog-priv.h"

//该文件必须包含在源文件中(*.cpp),以免宏展开时提示重复定义的错误



HybridMediaPlayer::HybridMediaPlayer() {
    mVideoStateInfo = new VideoStateInfo();
    mGLThread = new GLThread(mVideoStateInfo);
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
                AVMessage msg;
                msg.what = GL_MSG_DECODED_FIRST_FRAME;
                mVideoStateInfo->messageQueueGL->put(&msg);
                ALOGI("msg.what = GL_MSG_DECODED_FIRST_FRAME for test");
            }

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
