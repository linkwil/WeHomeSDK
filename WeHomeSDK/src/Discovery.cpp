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

#include "Discovery.h"

#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<sys/time.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ECLog.h"
#include "cJSON.h"

#include "platform.h"
#include "lwlRsa.h"

#define MAX_RECV_BUF_SIZE   4096
#define MAX_DEV_LIST_NUM    100

#ifdef _WIN32
#pragma pack(1)
typedef struct tagDiscoveryCmdHead
{
	char magic[4];
	int dataLen;
	char data[0];
} DiscoveryCmdHead;

typedef struct tagInitCmdHead
{
    char magic[4];
    int dataLen;
    char data[0];
} InitCmdHead;

#pragma pack()
#else
typedef struct tagDiscoveryCmdHead
{
	char magic[4];
	int dataLen;
	char data[0];
}__attribute__((packed)) DiscoveryCmdHead;


typedef struct tagInitCmdHead
{
    char magic[4];
    int dataLen;
    char data[0];
}__attribute__((packed)) InitCmdHead;

#endif

static int sIsSearchCanceled = 0;
static char sDiscoveryRespBuf[MAX_RECV_BUF_SIZE];
static int sRecvLen = 0;
static DeviceInfo sDevList[MAX_DEV_LIST_NUM];


//========== station config ===========
static int sIsStationConfigCanceled = 0;

static int sendProbe(int sockFd, char* bCastAddr)
{
	int ret = 0;
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;

	char probeBuf[256] = {0};
	DiscoveryCmdHead* pCmdHead = (DiscoveryCmdHead* )probeBuf;

	//===================================================
	// magic
	//===================================================
	strncpy( (char* )pCmdHead->magic, "EVC_", 4 );

    pJsonRoot = cJSON_CreateObject();

    //===================================================
    // ID
    //===================================================
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", 0/*DISCOVERY_CMD_DEV_SERACH_REQ*/);

    jsonStr = cJSON_Print(pJsonRoot);

	pCmdHead->dataLen = strlen(jsonStr)+1;
	memcpy( pCmdHead->data, jsonStr, pCmdHead->dataLen );

	struct sockaddr_in		addr;
	memset( &addr, 0x00, sizeof(struct sockaddr_in) );
	addr.sin_family = AF_INET;
	addr.sin_port = htons(10000);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	if( (ret=sendto( sockFd, probeBuf, sizeof(DiscoveryCmdHead)+pCmdHead->dataLen,
			0, (struct sockaddr *)&addr, sizeof(sockaddr))) < 0 )
	{
		LOGE("Send probe fail, ret:%d", ret);
	}

	if( (bCastAddr!=NULL) && strlen(bCastAddr) > 0 )
	{
		LOGI("Send cast:%s", bCastAddr);
		memset( &addr, 0x00, sizeof(sockaddr_in) );
		addr.sin_family = AF_INET;
		addr.sin_port = htons(10000);
		addr.sin_addr.s_addr = inet_addr(bCastAddr);

		if( (ret=sendto( sockFd, probeBuf, sizeof(DiscoveryCmdHead)+pCmdHead->dataLen,
				0, (struct sockaddr *)&addr, sizeof(sockaddr))) < 0 )
		{
			LOGE("Send probe fail, ret:%d", ret);
		}
	}

	LOGD("Send probe success, ret:%d", ret);

	return 0;
}

int parseData(char* data, int len)
{
	int ret = 0;

	if( sRecvLen+len > MAX_RECV_BUF_SIZE )
	{
		LOGE("dataLen:%d too long", len);
		return -1;
	}

	memcpy( sDiscoveryRespBuf+sRecvLen, data, len );
	sRecvLen += len;

	int parsedLen = 0;
	char* pParsePoint = NULL;
	int remainLen = 0;
	while( 1 )
	{
		pParsePoint = sDiscoveryRespBuf+parsedLen;
		remainLen = sRecvLen - parsedLen;
		if( (unsigned int)remainLen <= sizeof(DiscoveryCmdHead) )
		{
			//LOGD("No more data or parse");
			break;
		}

		DiscoveryCmdHead* pMsgHead = (DiscoveryCmdHead* )pParsePoint;
		if ( pMsgHead->magic[0] == 'E' &&
			 pMsgHead->magic[1] == 'V' &&
			 pMsgHead->magic[2] == 'C' &&
			 pMsgHead->magic[3] == '_' )
		{
			if( (pMsgHead->dataLen + sizeof(DiscoveryCmdHead)) > (unsigned int)remainLen ) // Msg not complete
			{
				break;
			}

			parsedLen += ((pMsgHead->dataLen + sizeof(DiscoveryCmdHead)));

            //LOGD("recv data:%s", pMsgHead->data);
            DeviceInfo dev;
            memset( &dev, 0x00, sizeof(DeviceInfo) );
            cJSON * pJson = cJSON_Parse(pMsgHead->data);
            if( pJson != NULL )
            {
                int cmdId = 0;
                cJSON * pItem = NULL;

                pItem = cJSON_GetObjectItem(pJson, "cmdId");
                if( pItem != NULL )
                {
                    cmdId = pItem->valueint;
                }

                if( cmdId == 1 )
                {
                    pItem = cJSON_GetObjectItem(pJson, "devType");
                    if( pItem != NULL )
                    {
                        dev.devType = pItem->valueint;
                    }
                    pItem = cJSON_GetObjectItem(pJson, "devName");
                    if( pItem != NULL )
                    {
                        strncpy( dev.devName, pItem->valuestring, 64 );
                    }
                    pItem = cJSON_GetObjectItem(pJson, "uid");
                    if( pItem != NULL )
                    {
                        strncpy( dev.uid, pItem->valuestring, 32 );
                    }
                    pItem = cJSON_GetObjectItem(pJson, "firmwareVer");
                    if( pItem != NULL )
                    {
                        strncpy( dev.fwVer, pItem->valuestring, 64 );
                    }

                    // Note: may received duplicate package
                    int i = 0;
                    for( i = 0; i < MAX_DEV_LIST_NUM; i++ )
                    {
                        if( !strncmp(sDevList[i].uid, dev.uid, 32) )
                        {
                            // duplicate
                            break;
                        }
                    }
                    if( i < MAX_DEV_LIST_NUM )
                    {
                        // update device info
                        memcpy( &sDevList[i], &dev, sizeof(DeviceInfo) );
                    }
                    else
                    {
                        for( i = 0; i < 100; i++ )
                        {
                            if( sDevList[i].uid[0] == 0 )
                            {
                                // Found new device
                                memcpy( &sDevList[i], &dev, sizeof(DeviceInfo) );
                                LOGD("Found new device:%s", dev.uid);
                                break;
                            }
                        }
                    }
                }
            }

		}else
			parsedLen++;
	}

	if( parsedLen >0 )
	{
		int remainDataLen = sRecvLen - parsedLen;
		char * pRemainData = sDiscoveryRespBuf + remainDataLen;
		memmove( sDiscoveryRespBuf, pRemainData, remainDataLen );
		sRecvLen = remainDataLen;
	}

	return ret;
}

int recvResponse(int sockFd, int timeout)
{
	int ret = 0;
	fd_set fdSet;
	struct timeval tv;
	int readBytes = 0;
	sRecvLen = 0;
	char buf[1000];
	struct  timeval tvCostTime;

#ifdef _WIN32
	ngx_gettimeofday(&tvCostTime);
#else
	gettimeofday(&tvCostTime, NULL);
#endif // _WIN32
    long long startTime = tvCostTime.tv_sec*1000 + tvCostTime.tv_usec/1000;

	memset( sDiscoveryRespBuf, 0x00, sizeof(sDiscoveryRespBuf) );

	while( 1 )
	{
		FD_ZERO( &fdSet );
		FD_SET( sockFd, &fdSet );
		tv.tv_sec = 0;
		tv.tv_usec = 50000; // 50ms
		ret = select( sockFd + 1, &fdSet, NULL, NULL, &tv);
		if( ret < 0 )
		{
			LOGE("select error");
			return -1;
		}
		else if( ret == 0 )
		{
			//LOGE("Recv response timeout");
		}
		else
		{
			readBytes = recv( sockFd, buf, 1000, 0 );
			if( readBytes <= 0 )
			{
				LOGE("Recv response fail, ret:%d", readBytes);
				break;
			}
			else
			{
				if( parseData( buf, readBytes ) < 0 )
				{
					break;
				}
			}
		}

		if( sIsSearchCanceled )
		{
		    LOGD("Search canceled");
		    break;
		}

#ifdef _WIN32
		ngx_gettimeofday( &tvCostTime );
#else
		gettimeofday(&tvCostTime, NULL);
#endif // _WIN32
        if( ((tvCostTime.tv_sec*1000+tvCostTime.tv_usec/1000)-startTime)>=timeout )
		{
			LOGE("Recv response timeout");
			break;
		}
	}

	return 0;
}


int getDevList(DeviceInfo* pDevInfo, int devInfoSize)
{
    int devCnt = 0;
    int i = 0;
    for( i = 0; i < (MAX_DEV_LIST_NUM>devInfoSize?devInfoSize:MAX_DEV_LIST_NUM); i++ )
    {
        if( strlen(sDevList[i].uid)>0 )
        {
            memcpy( pDevInfo+i, &sDevList[i], sizeof(DeviceInfo) );
            devCnt++;
        }
    }

    return devCnt;
}

int startDevSearch(int timeoutMS, char* bCastAddr)
{
    int sockFd = 0;

	sockFd = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sockFd <= 0 )
	{
		LOGE("create discovery socket fail");
		return -1;
	}

	int opt = true;
	if (setsockopt( sockFd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) != 0)
	{
		SOCKET_CLOSE( sockFd );
		return -1;
	}

	struct sockaddr_in sin;
	memset( &sin, 0x00, sizeof(sockaddr_in) );
	sin.sin_family = AF_INET;
	sin.sin_port = htons(10000);
	if ( 0 != bind( sockFd, (struct sockaddr *)&sin, sizeof(sockaddr_in)))
	{
		SOCKET_CLOSE( sockFd );
		return -1;
	}

	memset( &sDevList, 0x00, sizeof(sDevList) );

    sIsSearchCanceled = 0;

    int retryCnt = timeoutMS/1000;
    while( retryCnt-- > 0 )
    {
        if( sIsSearchCanceled )
        {
            LOGI("Dev search has been canceled");
            break;
        }
        if( sendProbe( sockFd, bCastAddr ) != 0 )
        {
            LOGE("sendProbe fail");
			SOCKET_CLOSE( sockFd );
            return -1;
        }

        if( recvResponse(sockFd, 1000) != 0 )
        {
            LOGE("recvResponse fail");
			SOCKET_CLOSE( sockFd );
            return -1;
        }
    }

	SOCKET_CLOSE( sockFd );

    return 0;
}

int stopDevSearch(void)
{
    sIsSearchCanceled = 1;

    return 0;
}




static int sendStationConfigData(int sockFd, const char*password, int timeZone, const char* bCastAddr)
{
    int ret = 0;
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;
    
    char configDataBuf[256] = {0};
    char afterEncodedDataBuf[256] = {0};
    InitCmdHead* pCmdHead = (InitCmdHead*)configDataBuf;
    
    //===================================================
    // magic
    //===================================================
    strncpy( (char* )pCmdHead->magic, "EVC_", 4 );
    
    pJsonRoot = cJSON_CreateObject();
    
    //===================================================
    // cmdId
    //===================================================
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", 0/*STATION_INIT_CMD_INIT*/);

    
    //===================================================
    // password
    //===================================================
    cJSON_AddStringToObject(pJsonRoot, "passwd", password);
    
    //===================================================
    // timeZone
    //===================================================
    cJSON_AddNumberToObject(pJsonRoot, "tz", timeZone);
    
    jsonStr = cJSON_Print(pJsonRoot);
    
    int afterEncodedDataBufLen = 256;
    int encRet = lwlrsa_encrypt((unsigned char*)jsonStr, (int)strlen(jsonStr), (unsigned char*)afterEncodedDataBuf, &afterEncodedDataBufLen);
    if (encRet == LWLRSA_RETURN_FAIL)
    {
        return -1;
    }
    
    
    pCmdHead->dataLen = afterEncodedDataBufLen+1;
    memcpy( pCmdHead->data, afterEncodedDataBuf, pCmdHead->dataLen );
    
    struct sockaddr_in        addr;
    memset( &addr, 0x00, sizeof(struct sockaddr_in) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    
    if( (ret = sendto( sockFd, configDataBuf, sizeof(InitCmdHead)+pCmdHead->dataLen,
                    0, (struct sockaddr *)&addr, sizeof(sockaddr))) < 0 )
    {
        LOGE("Send station config fail, ret:%d", ret);
    }
    
    if( (bCastAddr!=NULL) && strlen(bCastAddr) > 0 )
    {
        LOGI("Send cast:%s", bCastAddr);
        memset( &addr, 0x00, sizeof(sockaddr_in) );
        addr.sin_family = AF_INET;
        addr.sin_port = htons(10000);
        addr.sin_addr.s_addr = inet_addr(bCastAddr);
        
        if( (ret = sendto( sockFd, configDataBuf, sizeof(InitCmdHead)+pCmdHead->dataLen,
                        0, (struct sockaddr *)&addr, sizeof(sockaddr))) < 0 )
        {
            LOGE("Send station config fail, ret:%d", ret);
        }
    }
    
    LOGD("Send station config success, ret:%d", ret);
    
    return 0;
}

typedef struct tagStationConfigContextInfo
{
    char password[64];
    int timeZoneOffset;
    char bCastAddr[32];
}StationConfigContextInfo;
static void* stationConfigThreadFunc(void* param)
{
    StationConfigContextInfo* pContext = (StationConfigContextInfo* )param;
    int sockFd = 0;

    sockFd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sockFd <= 0 )
    {
        LOGE("create station config socket fail");
        return NULL;
    }

    int opt = true;
    if (setsockopt( sockFd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) != 0)
    {
        SOCKET_CLOSE( sockFd );
        return NULL;
    }

    struct sockaddr_in sin;
    memset( &sin, 0x00, sizeof(sockaddr_in) );
    sin.sin_family = AF_INET;
    sin.sin_port = htons(12345);
    if ( 0 != bind( sockFd, (struct sockaddr *)&sin, sizeof(sockaddr_in)))
    {
        SOCKET_CLOSE( sockFd );
        return NULL;
    }

    while( !sIsStationConfigCanceled )
    {
        if( sendStationConfigData( sockFd, pContext->password, pContext->timeZoneOffset, pContext->bCastAddr ) != 0 )
        {
            LOGE("sendStationConfigData fail");
            SOCKET_CLOSE( sockFd );
            return NULL;
        }

        usleep(100000);   //100ms
    }

    SOCKET_CLOSE( sockFd );

    if( pContext )
    {
        delete pContext;
    }

    return NULL;
}
int startStationConfig(const char* password, int timeZone, const char* bCastAddr)
{
    sIsStationConfigCanceled = 0;
    
    return 0;
}

int stopStationConfig(void)
{
    sIsStationConfigCanceled = 1;
    return 0;
}
 
