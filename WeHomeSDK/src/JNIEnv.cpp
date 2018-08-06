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

#include "JNIEnv.h"
#include "ECLog.h"

#ifdef _ANDROID

#include <map>
#include <pthread.h>

static std::map<int, void* > sJNIEnvMap;
static pthread_mutex_t mJNIEvnMutex;

void initJINEnv(void)
{
    pthread_mutex_init( &mJNIEvnMutex, NULL );
}

void addJNIEnv(int pid, void* env)
{
    pthread_mutex_lock( &mJNIEvnMutex );
    sJNIEnvMap[pid] = env;
    pthread_mutex_unlock( &mJNIEvnMutex );
}

void* getJNIEnv(int pid)
{
    void* env = 0;

    pthread_mutex_lock( &mJNIEvnMutex );
    std::map<int, void*>::iterator it = sJNIEnvMap.find(pid);
    if( it != sJNIEnvMap.end() )
    {
        env = it->second;
    }
    else
    {
        LOGE("Can not find env for pid:%d", pid);
    }
    pthread_mutex_unlock( &mJNIEvnMutex );

    return env;
}

void deleteJNIEnv(int pid)
{
    pthread_mutex_lock( &mJNIEvnMutex );
    sJNIEnvMap.erase( pid );
    pthread_mutex_unlock( &mJNIEvnMutex );
}

#endif