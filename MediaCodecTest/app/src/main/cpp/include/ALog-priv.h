/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//该文件必须包含在源文件中(*.cpp),以免宏展开时提示重复定义的错误
#ifndef NATIVEHELPER_ALOGPRIV_H_
#define NATIVEHELPER_ALOGPRIV_H_

#include <android/log.h>

#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif


/*
 * Basic log message macros intended to emulate the behavior of log/log.h
 * in system core.  This should be dependent only on ndk exposed logging
 * functionality.
 */

#ifndef ALOG
#define ALOG(priority, tag, fmt...) \
    __android_log_print(ANDROID_##priority, tag, fmt)
#endif

#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ((void)ALOG(LOG_VERBOSE, TAG, __VA_ARGS__))
#endif
#endif

#ifndef ALOGD
#if LOG_NDEBUG
#define ALOGD(...)   ((void)0)
#else
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, TAG, __VA_ARGS__))
#endif
#endif

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, TAG, __VA_ARGS__))
#endif

#endif


