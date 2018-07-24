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
#include <stdlib.h>
#include <string.h>
#include <algorithm> //algorithm need to be included before PPCS_API.h
#include <map>
#include <list>
#include <string>
#include <queue>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//#include <PPCS_API.h>
#include <LINK_API.h>

#endif

#include "CEasyCamClient.h"
#include "EasyCamSdk.h"
#include "LINK_API.h"
#include "ECLog.h"
#include "cJSON.h"
#include "EasyCamMsg.h"
#include "Discovery.h"
#include "JNIEnv.h"
#include "platform.h"

typedef struct tagKeepAliveServerInfo
{
	const char *ip;
	unsigned short port;
}KeepAliveServerInfo;

typedef struct tagKeepAliveServerTblInfo
{
	const char* key;
	const char* encDecKey;
	KeepAliveServerInfo server[3];
}KeepAliveServerTblInfo;

typedef struct tagWakeupQueryTaskInfo {
	pthread_t queryThread;
	char uid[32];
	OnlineQueryResultCallback resultCallback;
}WakeupQueryTaskInfo;

static std::map<std::string, WakeupQueryTaskInfo> sWakeupQueryTaskTbl;
static pthread_mutex_t sWakeupQueryTaskMutex;


typedef struct tagPPCSServerInfo
{
	const char* serverKey;
	const char* P2PInitString;
}PPCSServerInfo;

typedef struct tagSessionInfo
{
	int handle;
	CEasyCamClient* pClient;
	int refCnt;
}SessionInfo;

static bool sHasSdkInited = false;
static EC_INIT_INFO sECInitInfo;
static std::list<SessionInfo> sSessionList;
static int sSessionIndex = 0;
static pthread_mutex_t sSessionMutex;

static std::queue<int> sBrokenSessionQueue;
static pthread_mutex_t sBrokenSessionQueueMutex;

static pthread_t sGarbageCollectionThread;
static bool sGarbageCollectionThreadRun = false;

class CSessionCompare
{
private:
	int mHandle;
public:
	CSessionCompare(int handle) : mHandle(handle)
	{
	}
	bool operator()(SessionInfo &session)
	{
		if ((mHandle >= 0) && (session.handle == mHandle))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

static void* garbageCollectionThreadFunc(void* param)
{
	std::list<SessionInfo>::iterator it;

	LOGD("Enter garbageCollectionThreadFunc");

	while (sGarbageCollectionThreadRun)
	{
		int brokenSessionHandle = -1;
		pthread_mutex_lock(&sBrokenSessionQueueMutex);
		if (sBrokenSessionQueue.size() > 0)
		{
			brokenSessionHandle = sBrokenSessionQueue.front();
			sBrokenSessionQueue.pop();
		}
		pthread_mutex_unlock(&sBrokenSessionQueueMutex);

		if (brokenSessionHandle != -1) {
			pthread_mutex_lock(&sSessionMutex);
			for (it = sSessionList.begin(); it != sSessionList.end(); it++)
			{
				if ((it->handle == brokenSessionHandle) && (it->refCnt>0))
				{
					LOGI("Session:%d broken, decrease ref cnt", brokenSessionHandle);
					it->refCnt--;
				}
			}
			pthread_mutex_unlock(&sSessionMutex);
		}

		pthread_mutex_lock(&sSessionMutex);
		for (it = sSessionList.begin(); it != sSessionList.end(); )
		{
			if (it->refCnt == 0)
			{
				LOGD("Delete session, sessionHandle:%d", it->handle);

				if (it->pClient != NULL)
				{
					it->pClient->joinThreads();

					it->pClient->closeConnection();

					delete it->pClient;
				}
				it = sSessionList.erase(it);
			}
			else
			{
				it++;
			}
		}
		pthread_mutex_unlock(&sSessionMutex);

		usleep(100000); // 100ms
	}

	LOGD("Leave garbageCollectionThreadFunc");

	return NULL;
}

static void EC_OnConnectionBroken(int sessionHandle, int sockHandle, int issue)
{
	LOGI("EC_OnConnectionBroken, sessionHandle:%d, sockHandle:%d", sessionHandle, sockHandle);

	pthread_mutex_lock(&sBrokenSessionQueueMutex);
	sBrokenSessionQueue.push(sessionHandle);
	pthread_mutex_unlock(&sBrokenSessionQueueMutex);
}

static void EC_Audio_RecvData(int handle, char *data, int len, int payloadType,
	long long timestamp, int seq)
{
	if (sECInitInfo.lpAudio_RecvData != NULL)
	{
		sECInitInfo.lpAudio_RecvData(handle, data, len, payloadType, timestamp, seq);
	}
}

static void EC_Video_RecvData(int handle, char *data, int len, int payloadType,
	long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, unsigned int wifiQuality)
{
	if (sECInitInfo.lpVideo_RecvData != NULL)
	{
		sECInitInfo.lpVideo_RecvData(handle, data, len, payloadType, timestamp,
			seq, frameType, videoWidth, videoHeight, wifiQuality);
	}
}

static void EC_PBAudio_RecvData(int handle, char *data, int len, int payloadType,
	long long timestamp, int seq, int pbSessionNo)
{
	if (sECInitInfo.lpPBAudio_RecvData != NULL)
	{
		sECInitInfo.lpPBAudio_RecvData(handle, data, len, payloadType, timestamp, seq, pbSessionNo);
	}
}

static void EC_PBEnd(int handle, int pbSessionNo)
{
	if (sECInitInfo.lpPBEnd)
	{
		sECInitInfo.lpPBEnd(handle, pbSessionNo);
	}
}

static void EC_FileDownload_RecvData(int handle, char* data, int len, int sessionNo)
{
	if (sECInitInfo.lpFileDownload_RecvData)
	{
		sECInitInfo.lpFileDownload_RecvData(handle, data, len, sessionNo);
	}
}

static void EC_PBVideo_RecvData(int handle, char *data, int len, int payloadType,
	long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, int pbSessionNo)
{
	if (sECInitInfo.lpPBVideo_RecvData != NULL)
	{
		sECInitInfo.lpPBVideo_RecvData(handle, data, len, payloadType, timestamp,
			seq, frameType, videoWidth, videoHeight, pbSessionNo);
	}
}

static void EC_PIRData_RecvData(int handle, long long timeMS, short adcVal)
{
	if (sECInitInfo.lpPIRData_RecvData)
	{
		sECInitInfo.lpPIRData_RecvData(handle, timeMS, adcVal);
	}
}

static void EC_OnLoginResult(int handle, int errorCode, int seq, unsigned int notificationToken, unsigned int isCharging, unsigned int batPercent)
{
	LOGD("EC_OnLoginResult, handle:%d, errorCode:%d, seq:%d, notificationToken:%#X, isCharging:%d, bat:%d",
		handle, errorCode, seq, notificationToken, isCharging, batPercent);
	if (sECInitInfo.lpLoginResult)
	{
		sECInitInfo.lpLoginResult(handle, errorCode, seq, notificationToken, isCharging, batPercent);
	}
}

static void EC_OnCommandResult(int handle, char* data, int seq)
{
	LOGD("EC_OnCommandResult, seq:%d, data:%s", seq, data);
	if (sECInitInfo.lpCmdResult)
	{
		cJSON * pJson = cJSON_Parse(data);
		if (pJson != NULL)
		{
			int sessionCheckResult = -1;
			cJSON * pItem = NULL;
			pItem = cJSON_GetObjectItem(pJson, "result");
			if (pItem != NULL)
			{
				sessionCheckResult = pItem->valueint;
				if (sessionCheckResult != 0)
				{
					LOGE("Session check fail, ret:%d", sessionCheckResult);
					sECInitInfo.lpCmdResult(handle, data, CMD_EXEC_RESULT_AUTH_FAIL, seq);
				}
				else
				{
					sECInitInfo.lpCmdResult(handle, data, CMD_EXEC_RESULT_SUCCESS, seq);
				}
			}

			cJSON_Delete(pJson);
		}
		else
		{
			sECInitInfo.lpCmdResult(handle, data, CMD_EXEC_RESULT_FORMAT_ERROR, seq);
		}
	}
}

void EC_GetVersion(char* version)
{
	sprintf(version, "%s", "1.0.0.1");
}

int EC_Initialize(EC_INIT_INFO* init)
{
	if (sHasSdkInited)
	{
		LOGE("Had already inited");
		return -1;
	}

	setSdkLogLevel(EC_LOG_LEVEL_ALL);

	LINK_Initialize();

	UINT32 apiVer = LINK_GetAPIVersion();
	LOGD("PPCS_API Version: %d.%d.%d.%d",
		(apiVer & 0xFF000000) >> 24, (apiVer & 0x00FF0000) >> 16,
		(apiVer & 0x0000FF00) >> 8, (apiVer & 0x000000FF) >> 0);

	memset(&sECInitInfo, 0x00, sizeof(EC_INIT_INFO));
	if (init != NULL)
	{
		memcpy(&sECInitInfo, init, sizeof(EC_INIT_INFO));
	}

	pthread_mutex_init(&sSessionMutex, NULL);

	pthread_mutex_init(&sWakeupQueryTaskMutex, NULL);

	pthread_mutex_init(&sBrokenSessionQueueMutex, NULL);

#if _ANDROID
	initJINEnv();
#endif

	sGarbageCollectionThreadRun = true;
	pthread_create(&sGarbageCollectionThread, NULL, garbageCollectionThreadFunc, NULL);

	sHasSdkInited = true;

	return 0;
}

void EC_DeInitialize(void)
{
	if (sHasSdkInited)
	{
		// Logout all session
		std::list<SessionInfo>::iterator it;

		LOGD("Start join GarbageCollectionThread");
		sGarbageCollectionThreadRun = false;
		pthread_join(sGarbageCollectionThread, NULL);
		LOGD("GarbageCollectionThread joined");

		pthread_mutex_lock(&sSessionMutex);
		for (it = sSessionList.begin(); it != sSessionList.end(); it++)
		{
			it->pClient->logOut();
			delete it->pClient;
		}
		sSessionList.clear();
		pthread_mutex_unlock(&sSessionMutex);

		pthread_mutex_destroy(&sSessionMutex);

		LINK_DeInitialize();
	}
}

//query dev and wakeup server connect status
#define ERROR_NotLogin				-1
#define ERROR_InvalidParameter		-2
#define ERROR_SocketCreateFailed	-3
#define ERROR_SendToFailed			-4
#define ERROR_RecvFromFailed		-5
#define ERROR_UnKnown				-99

#define SERVER_NUM                  3

void *WakeUp_Query_ThreadFunc(void *arg)
{
#ifdef _ANDROID
    ATTACH_JNI_ENV(g_JVM, pthread_self());
#endif
    
    WakeupQueryTaskInfo *pQueryTaskInfo = (WakeupQueryTaskInfo*)arg;

    int LastLogin[3] = { -99, -99, -99 };
    
    int LastLoginNum = LINK_WakeUp_Query(pQueryTaskInfo->uid,
                                         3,
                                         2000,
                                         &LastLogin[0],
                                         &LastLogin[1],
                                         &LastLogin[2]);

    if (-1 == LastLoginNum) {
        LOGD("LastLogin=%d, Device not login!!", LastLoginNum);
        if (pQueryTaskInfo->resultCallback != NULL)
        {
            pQueryTaskInfo->resultCallback(ONLINE_QUERY_RESULT_SUCCESS,
                                           pQueryTaskInfo->uid, -1);
        }
    }
    else if (-99 == LastLoginNum) {
        LOGD("LastLogin=NoRespFromServer!!");
        if (pQueryTaskInfo->resultCallback != NULL)
        {
            pQueryTaskInfo->resultCallback(ONLINE_QUERY_RESULT_FAIL_SERVER_NO_RESP,
                                           pQueryTaskInfo->uid, -1);
        }
    }
    else if (0 <= LastLoginNum) {
        LOGD("LastLogin=%d [%d,%d,%d].", LastLoginNum,
             LastLogin[0], LastLogin[1], LastLogin[2]);
        if (pQueryTaskInfo->resultCallback != NULL)
        {
            pQueryTaskInfo->resultCallback(ONLINE_QUERY_RESULT_SUCCESS,
                                           pQueryTaskInfo->uid, LastLoginNum);
        }
    }
    else {
        LOGE("WakeUp_Query ret=%d, some error!!", LastLoginNum);
        if (pQueryTaskInfo->resultCallback != NULL)
        {
            pQueryTaskInfo->resultCallback(ONLINE_QUERY_RESULT_FAIL_UNKOWN,
                                           pQueryTaskInfo->uid, -1);
        }
    }
    
    pthread_mutex_lock(&sWakeupQueryTaskMutex);
    sWakeupQueryTaskTbl.erase(pQueryTaskInfo->uid);
    pthread_mutex_unlock(&sWakeupQueryTaskMutex);
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
    
    return NULL;
}


int EC_QueryOnlineStatus(const char* uid, OnlineQueryResultCallback callback)
{
    if (sWakeupQueryTaskTbl.find(uid) == sWakeupQueryTaskTbl.end())
    {
        pthread_mutex_lock(&sWakeupQueryTaskMutex);
        WakeupQueryTaskInfo queryTaskInfo;
        memset(&queryTaskInfo, 0x00, sizeof(WakeupQueryTaskInfo));
        strcpy(queryTaskInfo.uid, uid);
        queryTaskInfo.resultCallback = callback;
        sWakeupQueryTaskTbl[uid] = queryTaskInfo;
        pthread_create(&sWakeupQueryTaskTbl["uid"].queryThread, NULL,
                       WakeUp_Query_ThreadFunc, &sWakeupQueryTaskTbl[uid]);
        pthread_detach(sWakeupQueryTaskTbl["uid"].queryThread);
        pthread_mutex_unlock(&sWakeupQueryTaskMutex);
        
        return 0;
    }
    else
    {
        LOGE("Online query for uid:%s is running", uid);
        return -1;
    }
    
    return 0;
}

int EC_Login(const char* uid, const char* usrName, const char* password, const char* broadcastAddr,
	int seq, int needVideo, int needAudio, int connectType, int timeout)
{
	if (!sHasSdkInited) {
		LOGE("Please init sdk firstly");
		return -1;
	}
	SessionInfo session;
	session.handle = sSessionIndex;
	CLIENT_INIT_INFO init;
	init.lpConnectionBroken = EC_OnConnectionBroken;
	init.lpLoginResult = EC_OnLoginResult;
	init.lpCmdResult = EC_OnCommandResult;
	init.lpAudio_RecvData = EC_Audio_RecvData;
	init.lpVideo_RecvData = EC_Video_RecvData;
	init.lpPBAudio_RecvData = EC_PBAudio_RecvData;
	init.lpPBVideo_RecvData = EC_PBVideo_RecvData;
	init.lpPBEnd = EC_PBEnd;
	init.lpFileDownload_RecvData = EC_FileDownload_RecvData;
	init.lpPIRData_RecvData = EC_PIRData_RecvData;
	session.pClient = new CEasyCamClient(&init, session.handle, uid);
	session.refCnt = 1;

	pthread_mutex_lock(&sSessionMutex);
	sSessionList.push_back(session);
	session.pClient->logIn(uid, usrName, password, broadcastAddr, seq, needVideo, needAudio, connectType, (timeout - 1));
	pthread_mutex_unlock(&sSessionMutex);

	sSessionIndex++;

	LOGD("EC_Login, uid:%s, usrName:%s, password:%s, handle:%d", uid, usrName, password, session.handle);

	return session.handle;
}

int EC_Logout(int handle)
{
	if (!sHasSdkInited) {
		LOGE("Please init sdk firstly");
		return -1;
	}

	LOGD("EC_Logout, handle:%d, session cnt:%d", handle, sSessionList.size());

	pthread_mutex_lock(&sSessionMutex);
	std::list<SessionInfo>::iterator it = std::find_if(sSessionList.begin(),
		sSessionList.end(), CSessionCompare(handle));
	if (it != sSessionList.end())
	{
		if (it->pClient != NULL)
		{
			if (it->refCnt > 0)
			{
				it->pClient->logOut();
				it->refCnt--;
			}
			else
			{
				LOGI("Client ref cnt is 0");
			}
		}
	}
	else
	{
		LOGE("session:%d unexist", handle);
	}
	pthread_mutex_unlock(&sSessionMutex);

	LOGD("EC_Logout complete");

	return 0;
}

int EC_StartSearchDev(int timeoutMS, char* bCastAddr)
{
	return startDevSearch(timeoutMS, bCastAddr);
}

int EC_StopDevSearch(void)
{
	return stopDevSearch();
}

int EC_GetDevList(DeviceInfo* pDevInfo, int devInfoSize)
{
	return getDevList(pDevInfo, devInfoSize);
}

int EC_SendCommand(int handle, char* command, int seq)
{
	if (!sHasSdkInited) {
		LOGE("Please init sdk firstly");
		return -1;
	}

	int errCode = CMD_EXEC_RESULT_SUCCESS;

	pthread_mutex_lock(&sSessionMutex);
	std::list<SessionInfo>::iterator it = std::find_if(sSessionList.begin(),
		sSessionList.end(), CSessionCompare(handle));
	if (it != sSessionList.end())
	{
		if (it->pClient != NULL)
		{
			// add token saved internal
			cJSON * pJson = cJSON_Parse(command);
			if (pJson != NULL)
			{
				cJSON_AddNumberToObject(pJson, "token", it->pClient->getToken());
			}
			else
			{
				LOGE("Json parse fail:%s", command);
				errCode = CMD_EXEC_RESULT_FORMAT_ERROR;
			}

			char* pCmdWithTokenStr = NULL;
			pCmdWithTokenStr = cJSON_Print(pJson);
			cJSON_Minify(pCmdWithTokenStr);
			LOGD("pCmdWithTokenStr:%s", pCmdWithTokenStr);
			if (it->pClient->sendCommand(pCmdWithTokenStr, seq) != 0)
			{
				LOGE("Send command fail");
				errCode = CMD_EXEC_RESULT_SEND_FAIL;
			}
			if (pJson != NULL)
			{
				cJSON_Delete(pJson);
			}
		}
	}
	pthread_mutex_unlock(&sSessionMutex);

	if (errCode != CMD_EXEC_RESULT_SUCCESS)
	{
		if (sECInitInfo.lpCmdResult)
		{
			sECInitInfo.lpCmdResult(handle, command, errCode, seq);
		}
	}

	return 0;
}

int EC_SendTalkData(int handle, char* data, int dataLen, int payloadType, int seq)
{
	if (!sHasSdkInited) {
		LOGE("Please init sdk firstly");
		return -1;
	}

	pthread_mutex_lock(&sSessionMutex);
	std::list<SessionInfo>::iterator it = std::find_if(sSessionList.begin(),
		sSessionList.end(), CSessionCompare(handle));
	if (it != sSessionList.end())
	{
		if (it->pClient != NULL)
		{
			if (it->pClient->sendTalkData(data, dataLen, payloadType, seq) != 0)
			{
				// T.B.D
				LOGE("Send talk data fail");
			}
		}
	}
	pthread_mutex_unlock(&sSessionMutex);

	return 0;
}

int EC_Detect_Network_Type(const char* uid)
{
    if (!sHasSdkInited) {
        LOGE("Please init sdk firstly");
        return -1;
    }

    st_LINK_NetInfo netInfo;
    memset( &netInfo, 0x00, sizeof(st_LINK_NetInfo) );
    if( LINK_NetworkDetectByServer(&netInfo, 0, (CHAR*)uid) != ERROR_LINK_SUCCESSFUL )
    {
        LOGE("PPCS_NetworkDetectByServer failed");
        return EC_NAT_TYPE_UNKNOWN;
    }

    // NAT type, 0: Unknow, 1: IP-Restricted Cone type,   2: Port-Restricted Cone type, 3: Symmetric

    return netInfo.NAT_Type;
}

int EC_Subscribe(const char* uid, const char* appName,  const char* agName, const char* phoneToken, unsigned int eventCh)
{
    return LINK_Subscribe(uid, appName, agName, phoneToken, eventCh);
}

int EC_UnSubscribe(const char* uid, const char* appName,  const char* agName, const char* phoneToken, unsigned int eventCh)
{
    return LINK_UnSubscribe(uid, appName, agName, phoneToken, eventCh);
}

int EC_ResetBadge(const char* uid, const char* appName,  const char* agName, const char* phoneToken, unsigned int eventCh)
{
    return LINK_ResetBadge(uid, appName, agName, phoneToken, eventCh);
}

/*========================================================
   Only used for android JNI
========================================================*/
#ifdef _ANDROID
static std::map<int, void* > sVideoFrameJByteArrayMap;
static std::map<int, void* > sAudioFrameJByteArrayMap;
static std::map<int, void* > sPBVideoFrameJByteArrayMap;
static std::map<int, void* > sPBAudioFrameJByteArrayMap;
static std::map<int, void* > sFileDownloadJByteArrayMap;
static pthread_mutex_t sVideoFrameJByteArrayMapMutex;
static pthread_mutex_t sAudioFrameJByteArrayMapMutex;
static pthread_mutex_t sPBVideoFrameJByteArrayMapMutex;
static pthread_mutex_t sPBAudioFrameJByteArrayMapMutex;
static pthread_mutex_t sFileDownloadJByteArrayMapMutex;
extern "C" void initAVFrameJByteArray(void)
{
	pthread_mutex_init(&sVideoFrameJByteArrayMapMutex, NULL);
	pthread_mutex_init(&sAudioFrameJByteArrayMapMutex, NULL);
	pthread_mutex_init(&sPBVideoFrameJByteArrayMapMutex, NULL);
	pthread_mutex_init(&sPBAudioFrameJByteArrayMapMutex, NULL);
	pthread_mutex_init(&sFileDownloadJByteArrayMapMutex, NULL);
	sVideoFrameJByteArrayMap.clear();
	sAudioFrameJByteArrayMap.clear();
	sPBVideoFrameJByteArrayMap.clear();
	sPBAudioFrameJByteArrayMap.clear();
	sFileDownloadJByteArrayMap.clear();
}


extern "C" void* getVideoFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sVideoFrameJByteArrayMapMutex);
	std::map<int, void* >::iterator it = sVideoFrameJByteArrayMap.find(handle);
	if (it != sVideoFrameJByteArrayMap.end())
	{
		pthread_mutex_unlock(&sVideoFrameJByteArrayMapMutex);
		return it->second;
	}

	pthread_mutex_unlock(&sVideoFrameJByteArrayMapMutex);
	return NULL;
}

extern "C" void setVideoFrameJByteArray(int handle, void* byteArray)
{
	pthread_mutex_lock(&sVideoFrameJByteArrayMapMutex);
	sVideoFrameJByteArrayMap[handle] = byteArray;
	pthread_mutex_unlock(&sVideoFrameJByteArrayMapMutex);
}

extern "C" void deleteVideoFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sVideoFrameJByteArrayMapMutex);
	sVideoFrameJByteArrayMap.erase(handle);
	pthread_mutex_unlock(&sVideoFrameJByteArrayMapMutex);
}

extern "C" void* getAudioFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sAudioFrameJByteArrayMapMutex);
	std::map<int, void* >::iterator it = sAudioFrameJByteArrayMap.find(handle);
	if (it != sAudioFrameJByteArrayMap.end())
	{
		pthread_mutex_unlock(&sAudioFrameJByteArrayMapMutex);
		return it->second;
	}

	pthread_mutex_unlock(&sAudioFrameJByteArrayMapMutex);
	return NULL;
}

extern "C" void setAudioFrameJByteArray(int handle, void* byteArray)
{
	pthread_mutex_lock(&sAudioFrameJByteArrayMapMutex);
	sAudioFrameJByteArrayMap[handle] = byteArray;
	pthread_mutex_unlock(&sAudioFrameJByteArrayMapMutex);
}

extern "C" void deleteAudioFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sAudioFrameJByteArrayMapMutex);
	sAudioFrameJByteArrayMap.erase(handle);
	pthread_mutex_unlock(&sAudioFrameJByteArrayMapMutex);
}

extern "C" void* getPBVideoFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sPBVideoFrameJByteArrayMapMutex);
	std::map<int, void* >::iterator it = sPBVideoFrameJByteArrayMap.find(handle);
	if (it != sPBVideoFrameJByteArrayMap.end())
	{
		pthread_mutex_unlock(&sPBVideoFrameJByteArrayMapMutex);
		return it->second;
	}

	pthread_mutex_unlock(&sPBVideoFrameJByteArrayMapMutex);
	return NULL;
}

extern "C" void setPBVideoFrameJByteArray(int handle, void* byteArray)
{
	pthread_mutex_lock(&sPBVideoFrameJByteArrayMapMutex);
	sPBVideoFrameJByteArrayMap[handle] = byteArray;
	pthread_mutex_unlock(&sPBVideoFrameJByteArrayMapMutex);
}

extern "C" void deletePBVideoFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sPBVideoFrameJByteArrayMapMutex);
	sPBVideoFrameJByteArrayMap.erase(handle);
	pthread_mutex_unlock(&sPBVideoFrameJByteArrayMapMutex);
}

extern "C" void* getPBAudioFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sPBAudioFrameJByteArrayMapMutex);
	std::map<int, void* >::iterator it = sPBAudioFrameJByteArrayMap.find(handle);
	if (it != sPBAudioFrameJByteArrayMap.end())
	{
		pthread_mutex_unlock(&sPBAudioFrameJByteArrayMapMutex);
		return it->second;
	}

	pthread_mutex_unlock(&sPBAudioFrameJByteArrayMapMutex);
	return NULL;
}

extern "C" void setPBAudioFrameJByteArray(int handle, void* byteArray)
{
	pthread_mutex_lock(&sPBAudioFrameJByteArrayMapMutex);
	sPBAudioFrameJByteArrayMap[handle] = byteArray;
	pthread_mutex_unlock(&sPBAudioFrameJByteArrayMapMutex);
}

extern "C" void deletePBAudioFrameJByteArray(int handle)
{
	pthread_mutex_lock(&sPBAudioFrameJByteArrayMapMutex);
	sPBAudioFrameJByteArrayMap.erase(handle);
	pthread_mutex_unlock(&sPBAudioFrameJByteArrayMapMutex);
}

extern "C" void* getFileDownloadJByteArray(int handle)
{
	pthread_mutex_lock(&sFileDownloadJByteArrayMapMutex);
	std::map<int, void* >::iterator it = sFileDownloadJByteArrayMap.find(handle);
	if (it != sFileDownloadJByteArrayMap.end())
	{
		pthread_mutex_unlock(&sFileDownloadJByteArrayMapMutex);
		return it->second;
	}

	pthread_mutex_unlock(&sFileDownloadJByteArrayMapMutex);
	return NULL;
}

extern "C" void setFileDownloadJByteArray(int handle, void* byteArray)
{
	pthread_mutex_lock(&sFileDownloadJByteArrayMapMutex);
	sFileDownloadJByteArrayMap[handle] = byteArray;
	pthread_mutex_unlock(&sFileDownloadJByteArrayMapMutex);
}

extern "C" void deleteFileDownloadJByteArray(int handle)
{
	pthread_mutex_lock(&sFileDownloadJByteArrayMapMutex);
	sFileDownloadJByteArrayMap.erase(handle);
	pthread_mutex_unlock(&sFileDownloadJByteArrayMapMutex);
}
#endif
