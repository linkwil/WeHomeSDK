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

#ifndef ECLOG_H
#define ECLOG_H

#ifdef __cplusplus
extern "C"
{
#endif /* End of #ifdef __cplusplus */

#ifndef LOG_TAG
	#define LOG_TAG "EC-SDK"
#endif


enum ECAM_SDK_LOG_LEVEL
{
	EC_LOG_LEVEL_ALL,
	EC_LOG_LEVEL_INFO,
	EC_LOG_LEVEL_WARN,
	EC_LOG_LEVEL_ERROR,
	EC_LOG_LEVEL_NONE,
};

#ifdef _ANDROID
#include <android/log.h>

#define LOGE(format...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,format)
#define LOGI(format...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,format)
#define LOGD(format...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,format)

#else

#define LOGE(format, ...) printLog(EC_LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOGI(format, ...) printLog(EC_LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOGD(format, ...) printLog(EC_LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)

#endif

void setSdkLogLevel(int level);
void setLogFilePath(const char* path, int maxSize);

void printLog(int level, const char* file, int line, const char* format, ...);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */
    
#endif /*ECLOG_H*/
