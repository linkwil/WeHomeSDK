/*
 * WeHome SDK
 * Copyright (c) Shenzhen Linkwil Intelligent Technology Co. Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _JNIENV_H
#define _JNIENV_H

#ifdef _ANDROID
#include <jni.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif /* End of #ifdef __cplusplus */


#ifdef _ANDROID
void initJINEnv(void);
void addJNIEnv(int pid, void* env);
void* getJNIEnv(int pid);
void deleteJNIEnv(int pid);

#define ATTACH_JNI_ENV(jvm, pid)  { \
    JNIEnv* env; \
    jvm->AttachCurrentThread( &env, NULL ); \
    addJNIEnv(pid, (void* )env); \
}

#define DETACH_JNI_ENV(jvm, pid)  { \
    deleteJNIEnv(pid); \
    jvm->DetachCurrentThread(); \
}

extern JavaVM* g_JVM;
extern jclass g_CallbackCalss;
extern jmethodID g_methodID_OnLogInResult;
extern jmethodID g_methodID_OnCmdResult;
extern jmethodID g_methodID_OnAudio_RecvData;
extern jmethodID g_methodID_OnVideo_RecvData;

#else

#define ATTACH_JNI_ENV(jvm, env)
#define DETACH_JNI_ENV(jvm)

#endif

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif //_JNIENV_H

