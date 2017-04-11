//
// Created by Administrator on 2016/9/2.
//

#define LOG_NDEBUG 0
#define TAG "YTX-PLAYER-JNI"

#include "../../include/ALog-priv.h"
#include <string>
#include "../../gl_engine/include/gl_engine.h"
#include "../../include/android_media_YtxMediaPlayer.h"
#include "../../include/HybridMediaPlayer.h"
// 获取数组的大小
#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

// 指定要注册的类，对应完整的java类名

#define JNIREG_CLASS "ican/ytx/com/mediacodectest/media/player/pragma/HybridMediaPlayer"

static JavaVM *sVm;
// ----------------------------------------------------------------------------
/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv *env, const char *className, const char *msg) {
    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        ALOGE("Unable to find exception class %s", className);
        return -1;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        ALOGE("Failed throwing '%s' '%s'", className, msg);
    }
    return 0;
}


JNIEnv *getJNIEnv() {
    JNIEnv *env = NULL;
    if (sVm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("Failed to obtain JNIEnv");
        return NULL;
    }
    return env;
}

struct fields_t {
    jfieldID context;
    jfieldID surface_texture;
    jfieldID native_player;
    jmethodID post_event;
};
static fields_t fields;

class JNIMediaPlayerListener : public MediaPlayerListener {
public:
    JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz);

    ~JNIMediaPlayerListener();

    // virtual void notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
    virtual void notify(int msg, int ext1, int ext2);

private:
    pthread_mutex_t mLock;
    jclass mClass;     // Reference to MediaPlayer class
    jobject mObject;    // Weak ref to MediaPlayer Java object to call on
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz) {

    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    pthread_mutex_init(&mLock, NULL);
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find android/media/MediaPlayer");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass) env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener() {
    // remove global references
    JNIEnv *env = getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
    pthread_mutex_destroy(&mLock);
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2) {

    ALOGI("JNIMediaPlayerListener::notify IN\n");
    pthread_mutex_lock(&mLock);
    JNIEnv *env = NULL;
    if (getpid() != gettid()) {
        sVm->AttachCurrentThread(&env, NULL);
    } else {
        env = getJNIEnv();
    }

    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, NULL);

    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred while notifying an event.");
        env->ExceptionClear();
    }

    if (getpid() != gettid()) {
        sVm->DetachCurrentThread();
    }
    pthread_mutex_unlock(&mLock);
    ALOGI("JNIMediaPlayerListener::notify OUT\n");
}


static HybridMediaPlayer *getMediaPlayer(JNIEnv *env, jobject thiz) {

    HybridMediaPlayer *const p = (HybridMediaPlayer *) env->GetIntField(thiz, fields.context);
    return (p);
}

static HybridMediaPlayer *setMediaPlayer(JNIEnv *env, jobject thiz, const HybridMediaPlayer *player) {

    HybridMediaPlayer *old = (HybridMediaPlayer *) env->GetIntField(thiz, fields.context);
    env->SetIntField(thiz, fields.context, (int) player);
    return old;
}


JNIEXPORT void JNICALL android_media_player_native_init
        (JNIEnv *env, jclass mClazz) {

    jclass clazz;
    clazz = env->FindClass(JNIREG_CLASS);
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");


    if (fields.post_event == NULL) {
        return;
    }

    fields.surface_texture = env->GetFieldID(clazz, "mNativeSurfaceTexture", "I");
    if (fields.surface_texture == NULL) {
        return;
    }

}

JNIEXPORT void JNICALL android_media_player_native_setup
        (JNIEnv *env, jobject obj, jobject ytxMediaPlayer_weak_this) {


    HybridMediaPlayer *mPlayer = new HybridMediaPlayer();

    // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener *listener = new JNIMediaPlayerListener(env, obj,
                                                                  ytxMediaPlayer_weak_this);
    mPlayer->setListener(listener);
    setMediaPlayer(env, obj, mPlayer);

}

JNIEXPORT void JNICALL android_media_player_native_finalize
        (JNIEnv *env, jobject obj) {

}

JNIEXPORT void JNICALL android_media_player_native_message_loop
        (JNIEnv *env, jobject obj, jobject ytxMediaPlayer) {

}


void android_media_player_notifyRenderFrame(jobject obj) {

    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    jclass jclazz = env->GetObjectClass(obj);
    jmethodID jmtdId = env->GetMethodID(jclazz, "requestRender", "()V");
    env->CallVoidMethod(obj, jmtdId);

    sVm->DetachCurrentThread();

}


/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    _setGlSurface
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL android_media_player_setGlSurface
        (JNIEnv *env, jobject obj, jobject mVideoGlSurface) {

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    if (mPlayer->data.window) {
        ANativeWindow_release(mPlayer->data.window);
        mPlayer->data.window = NULL;
    }
    //ANativeWindow_fromSurface(env, mVideoGlSurfaceView);
    mPlayer->data.window = ANativeWindow_fromSurface(env, mVideoGlSurface);
   // ALOGI("@@@ setsurface %p", mPlayer->data.window);

}

JNIEXPORT void JNICALL android_media_player_setGlTexture
        (JNIEnv *env, jobject obj, jint texture,jobject mTextureSurface){
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->setTexture(texture);

    mPlayer->mVideoStateInfo->mTextureSurfaceObj = env->NewGlobalRef(mTextureSurface);
}


void android_media_player_updateSurface(jobject obj) {

    JNIEnv *env = NULL;
    sVm->AttachCurrentThread(&env, NULL);

    jclass jclazz = env->GetObjectClass(obj);
    jmethodID jmtdId = env->GetMethodID(jclazz, "updateSurfaceNative", "()V");
    env->CallVoidMethod(obj, jmtdId);

    sVm->DetachCurrentThread();

}
/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    died
 * Signature: ()V
 */
JNIEXPORT void JNICALL android_media_player_died
        (JNIEnv *env, jobject obj) {

}

JNIEXPORT void JNICALL android_media_player_native_requestRenderer
        (JNIEnv *env, jobject obj) {

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->rendererTexture();
}


/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    disconnect
 * Signature: ()V
 */
JNIEXPORT void JNICALL android_media_player_disconnect
        (JNIEnv *env, jobject obj) {

}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setDataSource
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL android_media_player_setDataSource
        (JNIEnv *env, jobject obj, jstring url) {

    const char *file_path = env->GetStringUTFChars(url, NULL);
    ALOGI("file_path=%s", file_path);
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->setDataSource(file_path);
    return 0;
}

JNIEXPORT jint JNICALL android_media_player_setSubtitles
        (JNIEnv *env, jobject obj, jstring url) {

    char *subtitle = (char *) env->GetStringUTFChars(url, NULL);
    ALOGI("subtitle=%s", subtitle);
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->setSubtitles(subtitle);
    return 0;
}



/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    prepare
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_prepare
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->prepare();
    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    prepareAsync
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_prepareAsync
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->prepareAsync();
    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    start
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_start
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->start();
    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    stop
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_stop
        (JNIEnv *env, jobject obj) {

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->stop();

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    release
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_release
        (JNIEnv *env, jobject obj) {

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->release();

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    pause
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_pause
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->pause();
    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    isPlaying
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_isPlaying
        (JNIEnv *env, jobject obj) {

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    return mPlayer->isPlaying();
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getVideoWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getVideoWidth
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    return mPlayer->getVideoWidth();
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getVideoHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getVideoHeight
        (JNIEnv *env, jobject obj) {
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    return mPlayer->getVideoHeight();
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    seekTo
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL android_media_player_seekTo
        (JNIEnv *env, jobject obj, jint msec) {

    // jlong <==> long long 使用%lld输出
    ALOGI("android_media_player_seekTo msec=%d\n", msec);
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    mPlayer->seekTo(msec);
    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getCurrentPosition
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getCurrentPosition
        (JNIEnv *env, jobject obj) {
    jint ret = 0;
    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    ret = mPlayer->getCurrentPosition();

    return ret;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getDuration
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getDuration
        (JNIEnv *env, jobject obj) {

    jint ret = 0;

    HybridMediaPlayer *mPlayer = getMediaPlayer(env, obj);
    ret = mPlayer->getDuration();

    return ret;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    reset
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_reset
        (JNIEnv *env, jobject obj) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setLooping
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL android_media_player_setLooping
        (JNIEnv *env, jobject obj, jint loop) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    isLooping
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_isLooping
        (JNIEnv *env, jobject obj) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setVolume
 * Signature: (FF)I
 */
JNIEXPORT jint JNICALL android_media_player_setVolume
        (JNIEnv *env, jobject obj, jfloat leftVolume, jfloat rightVolume) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setAudioSessionId
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL android_media_player_setAudioSessionId
        (JNIEnv *env, jobject obj, jint sessionId) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getAudioSessionId
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getAudioSessionId
        (JNIEnv *env, jobject obj) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setAuxEffectSendLevel
 * Signature: (F)I
 */
JNIEXPORT jint JNICALL android_media_player_setAuxEffectSendLevel
        (JNIEnv *env, jobject obj, jfloat level) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    attachAuxEffect
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL android_media_player_attachAuxEffect
        (JNIEnv *env, jobject obj, jint effectId) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    setRetransmitEndpoint
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL android_media_player_setRetransmitEndpoint
        (JNIEnv *env, jobject obj, jstring addrString, jlong port) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    updateProxyConfig
 * Signature: (Ljava/lang/String;ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL android_media_player_updateProxyConfig
        (JNIEnv *env, jobject obj, jstring host, jint port, jstring exclusionList) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    clear_l
 * Signature: ()V
 */
JNIEXPORT void JNICALL android_media_player_clear_l
        (JNIEnv *env, jobject obj) {

}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    seekTo_l
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL android_media_player_seekTo_l
        (JNIEnv *env, jobject obj, jint msec) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    prepareAsync_l
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_prepareAsync_l
        (JNIEnv *env, jobject obj) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    getDuration_l
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_getDuration_l
        (JNIEnv *env, jobject obj) {

    return 0;
}

/*
 * Class:     com_ytx_ican_media_player_YtxMediaPlayer
 * Method:    reset_l
 * Signature: ()I
 */
JNIEXPORT jint JNICALL android_media_player_reset_l
        (JNIEnv *env, jobject obj) {

    return 0;
}


// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {

        {"native_init",            "()V",                                      (void *) android_media_player_native_init},
        {"native_setup",           "(Ljava/lang/Object;)V",                    (void *) android_media_player_native_setup},
        {"native_finalize",        "()V",                                      (void *) android_media_player_native_finalize},
        {"native_message_loop",    "(Ljava/lang/Object;)V",                    (void *) android_media_player_native_message_loop},
        {"_requestRenderer",       "()V",                                      (void *) android_media_player_native_requestRenderer},
        {"_setGlSurface",          "(Ljava/lang/Object;)V",                    (void *) android_media_player_setGlSurface},
        {"_setGlTexture",          "(ILjava/lang/Object;)V",                   (void *) android_media_player_setGlTexture},
        {"_died",                  "()V",                                      (void *) android_media_player_died},
        {"_setSubtitles",          "(Ljava/lang/String;)I",                    (void *) android_media_player_setSubtitles},
        {"_setDataSource",         "(Ljava/lang/String;)I",                    (void *) android_media_player_setDataSource},
        {"_prepare",               "()I",                                      (void *) android_media_player_prepare},
        {"_prepareAsync",          "()I",                                      (void *) android_media_player_prepareAsync},
        {"_start",                 "()I",                                      (void *) android_media_player_start},
        {"_stop",                  "()I",                                      (void *) android_media_player_stop},
        {"_release",               "()I",                                      (void *) android_media_player_release},
        {"_pause",                 "()I",                                      (void *) android_media_player_pause},
        {"_isPlaying",             "()I",                                      (void *) android_media_player_isPlaying},
        {"_getVideoWidth",         "()I",                                      (void *) android_media_player_getVideoWidth},
        {"_getVideoHeight",        "()I",                                      (void *) android_media_player_getVideoHeight},
        {"_seekTo",                "(I)I",                                     (void *) android_media_player_seekTo},
        {"_getCurrentPosition",    "()I",                                      (void *) android_media_player_getCurrentPosition},
        {"_getDuration",           "()I",                                      (void *) android_media_player_getDuration},
        {"_reset",                 "()I",                                      (void *) android_media_player_reset},
        {"_setLooping",            "(I)I",                                     (void *) android_media_player_setLooping},
        {"_isLooping",             "()I",                                      (void *) android_media_player_isLooping},
        {"_setVolume",             "(FF)I",                                    (void *) android_media_player_setVolume},
        {"_setAudioSessionId",     "(I)I",                                     (void *) android_media_player_setAudioSessionId},
        {"_getAudioSessionId",     "()I",                                      (void *) android_media_player_getAudioSessionId},
        {"_setAuxEffectSendLevel", "(F)I",                                     (void *) android_media_player_setAuxEffectSendLevel},
        {"_attachAuxEffect",       "(I)I",                                     (void *) android_media_player_attachAuxEffect},
        {"_setRetransmitEndpoint", "(Ljava/lang/String;J)I",                   (void *) android_media_player_setRetransmitEndpoint},
        {"_updateProxyConfig",     "(Ljava/lang/String;ILjava/lang/String;)I", (void *) android_media_player_updateProxyConfig},
        {"_clear_l",               "()V",                                      (void *) android_media_player_clear_l},
        {"_seekTo_l",              "(I)I",                                     (void *) android_media_player_seekTo_l},
        {"_prepareAsync_l",        "()I",                                      (void *) android_media_player_prepareAsync_l},
        {"_getDuration_l",         "()I",                                      (void *) android_media_player_getDuration_l},
        {"_reset_l",               "()I",                                      (void *) android_media_player_reset_l}


};


// 注册native方法到java中
static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


int register_android_media_player_ytx(JNIEnv *env) {
    // 调用注册方法
    return registerNativeMethods(env, JNIREG_CLASS,
                                 gMethods, NELEM(gMethods));
}


jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jint result = -1;
    sVm = vm;
    ALOGI("INF: sVm=%d\n", sVm);
    if (sVm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);


    if (register_android_media_player_ytx(env) < 0) {
        ALOGE("ERROR: YTX mediaPlayer native registration failed\n");
        goto bail;
    }
    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

    bail:
    return result;
}