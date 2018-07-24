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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include "ECLog.h"
#include "platform.h"


static int ecamSDKLogLevel = EC_LOG_LEVEL_ERROR;
static char ecamSDKLogFilePath[128] = {0};
static FILE* fpLogFile = NULL;
static int curLogFileIndex = 1;
static int maxLogFileSize = 5*1024*1024;

#define LOG_STRING_LEN		128

void setSdkLogLevel(int level)
{
	ecamSDKLogLevel = level;
}

void setLogFilePath(const char* path, int maxSize)
{
	if( path != NULL )
	{
		strncpy( ecamSDKLogFilePath, path, 128 );
		maxLogFileSize = maxSize;
		if( fpLogFile != NULL )
		{
			fclose( fpLogFile );
		}
		char ecamSDKLogFilePathReal[128];
		snprintf(ecamSDKLogFilePathReal, 128, "%s.2", ecamSDKLogFilePath);
#ifdef _WIN32
		if( _access(ecamSDKLogFilePathReal, F_OK) != 0)
#else
		if( access(ecamSDKLogFilePathReal, F_OK) != 0 )
#endif
		{
			snprintf(ecamSDKLogFilePathReal, 128, "%s.1", ecamSDKLogFilePath);
			fpLogFile = fopen( ecamSDKLogFilePathReal, "ab+");
			if( fpLogFile == NULL )// Create a new file if open failed
			{
				fpLogFile = fopen( ecamSDKLogFilePathReal, "wb+");
				if( fpLogFile == NULL )
				{
					printf("Can't open log file:%s\n", ecamSDKLogFilePathReal);
				}
				else
				{
					curLogFileIndex = 1;
					printf("Open log file:%s\n", ecamSDKLogFilePathReal);
				}
			}
		}
		else
		{
			struct stat log1FileStat;
			struct stat log2FileStat;
			stat( ecamSDKLogFilePathReal, &log1FileStat );

			snprintf(ecamSDKLogFilePathReal, 128, "%s.2", ecamSDKLogFilePath);
			stat( ecamSDKLogFilePathReal, &log2FileStat );

#ifdef _ANDROID
            		if( log1FileStat.st_mtime > log2FileStat.st_mtime )
#else
#ifdef _WIN32
			if(log1FileStat.st_mtime > log2FileStat.st_mtime )
#else
#ifdef LINUX
			if(log1FileStat.st_mtime > log2FileStat.st_mtime )
#else
           		 if( log1FileStat.st_mtimespec.tv_nsec > log1FileStat.st_mtimespec.tv_nsec )
#endif
#endif
#endif
			{
				snprintf(ecamSDKLogFilePathReal, 128, "%s.1", ecamSDKLogFilePath);
				fpLogFile = fopen( ecamSDKLogFilePathReal, "ab+");
				if( fpLogFile == NULL )// Create a new file if open failed
				{
					curLogFileIndex = 1;
					printf("Open log file:%s\n", ecamSDKLogFilePathReal);
				}
			}
			else
			{
				snprintf(ecamSDKLogFilePathReal, 128, "%s.2", ecamSDKLogFilePath);
				fpLogFile = fopen( ecamSDKLogFilePathReal, "ab+");
				if( fpLogFile == NULL )// Create a new file if open failed
				{
					printf("Open log file:%s\n", ecamSDKLogFilePathReal);
				}
				else
				{
					curLogFileIndex = 2;
				}
			}
		}

		printf("Open log file:%s\n", ecamSDKLogFilePathReal);
	}
}

void printLog(int level, const char* file, int line, const char* format, ...)
{
	if( level >=  ecamSDKLogLevel)
	{
		char logTime[64] = {0};
		char strPrefix[128] = {0};
		char strSuffix[128] = {0};

		if( fpLogFile != NULL )
		{
			if( ftell(fpLogFile) >= maxLogFileSize )
			{
				printf("-----------curLogFileIndex:%d\n", curLogFileIndex);
				if( curLogFileIndex == 1 )
				{
					curLogFileIndex = 2;
				}
				else
				{
					curLogFileIndex = 1;
				}

				if( fpLogFile != NULL )
				{
					fclose( fpLogFile );
				}
				char ecamSDKLogFilePathReal[128];
				snprintf(ecamSDKLogFilePathReal, 128, "%s.%d", ecamSDKLogFilePath, curLogFileIndex);
				fpLogFile = fopen( ecamSDKLogFilePathReal, "wb+");
				if( fpLogFile == NULL )
				{
					printf("Can't open log file\n");
				}
				printf("-----------ChangeTo:%s\n", ecamSDKLogFilePathReal);
			}
		}

	    // if is error msg, then print color msg.
	    // \033[31m : red text code in shell
	    // \033[32m : green text code in shell
	    // \033[33m : yellow text code in shell
	    // \033[0m : normal text code
#ifndef _WIN32
	    if( level == EC_LOG_LEVEL_WARN )
	    {
	    	printf("\033[33m");
	    }
	    else if( level == EC_LOG_LEVEL_ERROR )
	    {
	    	printf("\033[31m"); // Red
	    }
#endif

		int strLen = 0;
		strLen += printf( "[SDK]");

	    // clock time
		timeval tv;
#ifdef _WIN32
		ngx_gettimeofday(&tv);
#else
	    if (gettimeofday(&tv, NULL) == -1) {
	        return;
	    }
#endif

	    // to calendar time
	    struct tm* tm;
		time_t curTime = tv.tv_sec;
	    if ((tm = localtime(&curTime)) == NULL) {
	        return;
	    }

	    snprintf(logTime, 64, "[%d-%02d-%02d %02d:%02d:%02d.%03d]",
	                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
	                tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000) );

		strLen += printf( "%s ", logTime );
		if( fpLogFile != NULL )
		{
			strncat( strPrefix, logTime, 64 );
		}

		if( level == EC_LOG_LEVEL_ERROR ){
			strLen += printf("[ERROR]");
			if( fpLogFile != NULL )
			{
				strncat( strPrefix, "[ERROR]", 64 );
			}
		}else if( level == EC_LOG_LEVEL_WARN ){
			strLen += printf("[WARN ]");
			if( fpLogFile != NULL )
			{
				strncat( strPrefix, "[WARN ]", 64 );
			}
		}else if( level == EC_LOG_LEVEL_INFO ){
			strLen += printf("[INFO ]");
			if( fpLogFile != NULL )
			{
				strncat( strPrefix, "[INFO ]", 64 );
			}
		}

		if( fpLogFile != NULL )
		{
			fputs( strPrefix, fpLogFile );
		}

		va_list args;
		va_start( args, format );
		strLen += vprintf( format, args );
		if( fpLogFile != NULL )
		{
			vfprintf( fpLogFile, format, args );
		}
		va_end( args );

		while( strLen < LOG_STRING_LEN )
		{
			strLen++;
			printf(" ");
		}

		const char* pFileDir = strrchr(file, '\\');
		if (pFileDir != NULL)
		{
			printf("------%s,%d\n", pFileDir+1, line);
		}
		else
		{
			printf("------%s,%d\n", file, line);
		}
		if( fpLogFile != NULL )
		{
			snprintf( strSuffix, 128, "------%s,%d\n", file, line );
			fputs( strSuffix, fpLogFile );
			fflush( fpLogFile );
		}

#ifndef _WIN32
		printf("\033[0m");
#endif
	}
}

