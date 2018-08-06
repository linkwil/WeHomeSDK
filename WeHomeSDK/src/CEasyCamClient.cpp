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

#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#endif

#ifndef _IOS
#ifndef _WIN32
#include <sys/prctl.h>
#endif
#endif

#include "CEasyCamClient.h"
#include "ECLog.h"
#include "EasyCamMsg.h"
#include "cJSON.h"
#include "EasyCamSdk.h"
#include "JNIEnv.h"
#include "platform.h"

#include "LINK_API.h"


#define MAX_CMD_QUEUE_SIZE 			(20)
#define MAX_TALK_DATA_QUEUE_SIZE    (10)

#define PLAY_RECORD_FRAME_TYPE_START    (2)
#define PLAY_RECORD_FRAME_TYPE_END      (4)

#define EASYCAM_AUDIO_FORMAT_PCM			(0)
#define EASYCAM_AUDIO_FORMAT_ADPCM			(1)

#ifdef _WIN32
#pragma pack(1)
typedef struct tagFrameHeadInfo
{
	unsigned long long seqNo;
	long long timeStamp;   // in us
	unsigned int videoLen;
	unsigned int audioLen;
	unsigned int isKeyFrame;
	unsigned int videoWidth;
	unsigned int videoHeight;
	unsigned int audio_format;
	unsigned int frameType;
	unsigned int wifiQuality;
	int pbSessionNo;
	unsigned char reserved[52];
	char data[0];
} FrameHeadInfo;
#pragma pack()
#else
typedef struct tagFrameHeadInfo
{
	unsigned long long seqNo;
	long long timeStamp;   // in us
	unsigned int videoLen;
	unsigned int audioLen;
	unsigned int isKeyFrame;
	unsigned int videoWidth;
	unsigned int videoHeight;
	unsigned int audio_format;
	unsigned int frameType;
	unsigned int wifiQuality;
    int pbSessionNo;
	unsigned char reserved[52];
	char data[0];
}__attribute__((packed))  FrameHeadInfo;
#endif

#define BUILD_CMD_MSG(msg, cmdStr, seq) \
{ \
	msg = new char[strlen(cmdStr)+1+sizeof(EasyCamMsgHead)]; \
	memset( msg, 0x00, (strlen(cmdStr)+1+sizeof(EasyCamMsgHead)) ); \
	EasyCamMsgHead* pCmdMsgHead = (EasyCamMsgHead* )msg; \
	strncpy( pCmdMsgHead->magic, "LINKWIL", 8 ); \
	pCmdMsgHead->msgId = EC_MSG_ID_CMD;	\
	pCmdMsgHead->seq = seq;	\
	pCmdMsgHead->dataLen = strlen(cmdStr)+1; \
	strcpy( pCmdMsgHead->data, cmdStr ); \
}

#define BUILD_TALK_PACK(pack, talkData, talkDataLen, payloadType, seq) \
{ \
	pack = new char[talkDataLen+sizeof(EasyCamMsgHead)+sizeof(FrameHeadInfo)]; \
	EasyCamMsgHead* pCmdMsgHead = (EasyCamMsgHead* )pack; \
	FrameHeadInfo* pFrameHead = (FrameHeadInfo* )pCmdMsgHead->data; \
	strncpy( pCmdMsgHead->magic, "LINKWIL", 8 ); \
	pCmdMsgHead->msgId = EC_MSG_ID_TALK_DATA;	\
	pCmdMsgHead->seq = seq;	\
	pCmdMsgHead->dataLen = talkDataLen+sizeof(FrameHeadInfo); \
	pFrameHead->audioLen = talkDataLen; \
	pFrameHead->videoLen = 0; \
	pFrameHead->audio_format = payloadType; \
	memcpy( pFrameHead->data, talkData, talkDataLen ); \
}

const char *getP2PErrorCodeInfo(int err)
{
    if (0 < err)
        return "NoError";
    switch (err)
    {
        case 0: return "ERROR_P2P_SUCCESSFUL";
        case -1: return "ERROR_P2P_NOT_INITIALIZED";
        case -2: return "ERROR_P2P_ALREADY_INITIALIZED";
        case -3: return "ERROR_P2P_TIME_OUT";
        case -4: return "ERROR_P2P_INVALID_ID";
        case -5: return "ERROR_P2P_INVALID_PARAMETER";
        case -6: return "ERROR_P2P_DEVICE_NOT_ONLINE";
        case -7: return "ERROR_P2P_FAIL_TO_RESOLVE_NAME";
        case -8: return "ERROR_P2P_INVALID_PREFIX";
        case -9: return "ERROR_P2P_ID_OUT_OF_DATE";
        case -10: return "ERROR_P2P_NO_RELAY_SERVER_AVAILABLE";
        case -11: return "ERROR_P2P_INVALID_SESSION_HANDLE";
        case -12: return "ERROR_P2P_SESSION_CLOSED_REMOTE";
        case -13: return "ERROR_P2P_SESSION_CLOSED_TIMEOUT";
        case -14: return "ERROR_P2P_SESSION_CLOSED_CALLED";
        case -15: return "ERROR_P2P_REMOTE_SITE_BUFFER_FULL";
        case -16: return "ERROR_P2P_USER_LISTEN_BREAK";
        case -17: return "ERROR_P2P_MAX_SESSION";
        case -18: return "ERROR_P2P_UDP_PORT_BIND_FAILED";
        case -19: return "ERROR_P2P_USER_CONNECT_BREAK";
        case -20: return "ERROR_P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY";
        case -21: return "ERROR_P2P_INVALID_APILICENSE";
        case -22: return "ERROR_P2P_FAIL_TO_CREATE_THREAD";
        default:
            return "Unknow, something is wrong!";
    }
}

CEasyCamClient::CEasyCamClient(CLIENT_INIT_INFO* init, int sessionHandle, const char* devUid) {

	pthread_mutex_init( &mCmdQueueMutex, NULL );
	pthread_mutex_init( &mCmdMsgMutex, NULL );
	pthread_mutex_init( &mTalkDataQueueMutex, NULL );
	pthread_mutex_init( &mJNIEnvMutex, NULL );
	mSockHandle = -1;
	mSessionHandle = sessionHandle;
    
    mIsConnecting = false;
    mLanIsConnecting = false;
    mP2pIsConnecting = false;
    mRelayIsConnecting = false;
    mConnectTimeRuning = false;
    mLanSockHandle = -1;
    mP2pSockHandle = -1;
    mRelaySockHandle = -1;

	INIT_THREAD_ID(mLanConnectThread);
	INIT_THREAD_ID(mP2pConnectThread);
	INIT_THREAD_ID(mRelayConnectThread);
	INIT_THREAD_ID(mConnectTimeThread);
    INIT_THREAD_ID(mCmdRecvThread);

    mLanIsConnectSuccess = false;
    mP2pIsConnectSuccess = false;
    mRelayIsConnectSuccess = false;

    mToken = 0;
    memset( &mAdpcmDecState, 0x00, sizeof(adpcm_state) );
    memset( &mAdpcmEncState, 0x00, sizeof(adpcm_state) );

	memcpy( &mInitInfo, init, sizeof(CLIENT_INIT_INFO) );

	memset( mUid, 0x00, sizeof(mUid) );
	strncpy( mUid, devUid, 64 );

	srand((unsigned)time(NULL));

	mNeedBreakConnect = false;

	mNsBufSize = 0;
    WebRtcNs_Create( &mNsHandle );
    WebRtcNs_Init( mNsHandle, 16000 );
    WebRtcNs_set_policy( mNsHandle, 2 );

    WebRtcAgc_Create(&mAgcHandle);
    int minLevel = 0;
    int maxLevel = 255;
    int agcMode  = kAgcModeFixedDigital;
    WebRtcAgc_Init(mAgcHandle, minLevel, maxLevel, agcMode, 16000);

    WebRtcAgc_config_t agcConfig;
    agcConfig.compressionGaindB = 20;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 3;
    WebRtcAgc_set_config(mAgcHandle, agcConfig);
    mMicLevelIn = 0;
    mMicLevelOut = 0;

	mCmdSendThreadRun = true;
	pthread_create( &mCmdSendThread, NULL, cmdSendThread, this );
	mHasCmdSendThreadStarted = true;

	mTalkDataSendThreadRun = true;
	pthread_create( &mTalkDataSendThread, NULL, audioSendThread, this );
    mHasTalkDataSendThreadStarted = true;



	mAVRecvThreadRun = false;
	mHasAVRecvThreadStarted  = false;
	mPBRecvThreadRun = false;
	mHasPBRecvThreadStarted = false;
    mFileDownloadThreadRun = false;
    mHasFileDownloadThreadStarted = false;
	mCmdQueueEnable = true;
	mTalkDataQueueEnable = true;
    mHasCmdRecvThreadStarted = false;

	mHasBrokenDetected = false;

	mNeedWaitPlaybackStartFrame = false;
}

CEasyCamClient::~CEasyCamClient() {

    if( mHasTalkDataSendThreadStarted )
    {
        LOGD("Start join talk data send thread");
        mTalkDataSendThreadRun = false;
        pthread_join(mTalkDataSendThread, NULL);
        mHasTalkDataSendThreadStarted = false;
        LOGD("talk data send thread joined");
    }

    if( mHasCmdSendThreadStarted )
    {
        LOGD("Start join cmd send thread");
        mCmdSendThreadRun = false;
        pthread_join(mCmdSendThread, NULL);
        LOGD("cmd send thread joined");
        mHasCmdSendThreadStarted = false;
    }

    if( mNsHandle )
    {
        WebRtcNs_Free( mNsHandle );
    }

    if( mAgcHandle )
    {
        WebRtcAgc_Free(mAgcHandle);
    }

	pthread_mutex_destroy( &mCmdQueueMutex );
	pthread_mutex_destroy( &mCmdMsgMutex );
	pthread_mutex_destroy( &mJNIEnvMutex );
	pthread_mutex_destroy( &mTalkDataQueueMutex );
}

int CEasyCamClient::logIn(const char* uid, const char* usrName, const char* password, const char* broadcastAddr, int seq, int needVideo, int needAudio, int connectType, int timeout)
{
    strncpy( mUid, uid, 64 );
    strncpy( mUsrAccount, usrName, 64 );
    strncpy( mUsrPassword, password, 64 );
    memset( mBroadcastAddr, 0, 64);
    strncpy( mBroadcastAddr, broadcastAddr, 64);
    mLoginSeq = seq;
    mLoginNeedVideo = needVideo;
    mLoginNeedAudio = needAudio;
    mConnectType = connectType;
    mConnectTimeoutValue = timeout;
    
    mLanIsConnecting = false;
    mP2pIsConnecting = false;
    mRelayIsConnecting = false;
    mConnectTimeRuning = false;
    mBroadcastThreadRun = true;
    
    pthread_create(&mBroadcastThread, NULL, broadcastThread, this); 
    
    if (connectType & CONNECT_TYPE_LAN)
    {
		pthread_create(&mLanConnectThread, NULL, lanConnectThread, this);
    }
    
    if (connectType & CONNECT_TYPE_P2P)
    {
		pthread_create(&mP2pConnectThread, NULL, p2pConnectThread, this);
    }
    
    if (connectType & CONNECT_TYPE_RELAY)
    {
		pthread_create(&mRelayConnectThread, NULL, relayConnectThread, this);
    }
    
    pthread_create(&mConnectTimeThread, NULL, connectTimeThread, this);
    
    return 0;
}

void* CEasyCamClient::lanConnectThread(void* param)
{
    int ret;
    CEasyCamClient* pThis = (CEasyCamClient* )param;

#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"lanConnectThread");
#endif
#endif

    
    //LOGD("Attach connect thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("Connect thread env attached");
    
    /*
     bEnableLanSearch:
     
     Bit 0 [LanSearch] , 0: Disable Lan search, 1: Enable Lan Search
     Bit 1~4 [P2P Try time]:
     0 (0b0000): 5 second (default)
     15 (0b1111): 0 second, No P2P trying
     Bit 5 [RelayOff], 0: Relay mode is allowed, 1: No Relay connection
     Bit 6 [ServerRelayOnly], 0: Device Relay is allowed, 1: Only Server relay (if Bit 5 = 1, this value is ignored)
     */
    
    int isNeedLan = 1;
    int isNeedP2p = 0;
    int isNeedRelay = 0;
    CHAR bEnableLanSearch = 0;
    bEnableLanSearch |= isNeedLan;
    if (isNeedP2p)
    {
        bEnableLanSearch |= 0x0;
    }else
    {
        bEnableLanSearch |= 0x1E;
    }
    
    if (isNeedRelay)
    {
        bEnableLanSearch |= (1 << 6);
    }else
    {
        bEnableLanSearch |= (1 << 5);
    }
    
    
    LOGD("Start lan connect to:%s, connectType:%d, bEnableLanSearch:%8x", pThis->mUid, pThis->mConnectType, bEnableLanSearch);
    
CONNECT_REMOTE:
    pThis->mLanSockHandle = -1;
    pThis->mLanIsConnecting = true;
    int randomPort = rand()%60000+2048;
    LOGD("Connect by lan, port:%d", randomPort);
    ret = LINK_ConnectByServer(pThis->mUid, bEnableLanSearch, randomPort);
    pThis->mLanIsConnecting = false;
    if( ret < 0 )
    {
        // Connect fail
        LOGE("Lan Connect to:%s fail, ret:%d, error:%s", pThis->mUid, ret, getP2PErrorCodeInfo(ret) );
        
        if( pThis->mNeedBreakConnect || (ret==ERROR_LINK_USER_CONNECT_BREAK) )

        {
			//Don't callback if interrupted by user
        }
        else
        {
            LOGI("Remote is offline, retry...");
            goto CONNECT_REMOTE;
        }
        
        pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
        DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
        pthread_mutex_unlock( &pThis->mJNIEnvMutex );
        
        LOGD("lanConnectThread quit by self");
        return NULL;
    }
    else
    {
        st_LINK_Session Sinfo;

        if(LINK_Check(ret, &Sinfo) == ERROR_LINK_SUCCESSFUL)
        {
            LOGD("Lan connect to remote success, mode:%s, cost time:%d, localAddr:(%s:%d), remoteAddr:(%s:%d)",
                 ((Sinfo.bMode ==0)? "P2P":"RLY"), Sinfo.ConnectTime,
                 inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port),
                 inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
        }
        
        
        int isGetAck = 0;
        char recvBuf[16] = {0};
        int recvSize = 3;
        int recvTimeIndex = 0;
        int recvDataIndex = 0;
        LOGD("Lan connect to remote success, start get ACK.");
        while (!pThis->mNeedBreakConnect)
        {
            recvSize = 3;
            recvSize -= recvDataIndex;
            LINK_Read(ret, 0, recvBuf+recvDataIndex, &recvSize, 10);        //10ms
            
            recvDataIndex += recvSize;
            if (recvDataIndex == 3)
            {
                if (0 == strcmp(recvBuf, "ACK"))
                {
                    isGetAck = 1;
                    break;
                }
            }
            
            recvTimeIndex ++;
            if (recvTimeIndex >= 500)                                       // 5s
            {
                LOGI("lan recv ack timeout, retry...");
                goto CONNECT_REMOTE;
            }
            
        }
        
        LOGD("lan quit get ack while, isGetAck:%d", isGetAck);
        
        if (isGetAck)
        {
            LOGD("lan isGetAck:%d, Relay sock:%d", isGetAck, ret);
            pThis->mLanSockHandle = ret;
            pThis->mLanIsConnectSuccess = true;
        }else{
            LINK_ForceClose(ret);
        }
        
    }
    
    
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    
    return NULL;
}

void* CEasyCamClient::p2pConnectThread(void* param)
{
    int ret;
    CEasyCamClient* pThis = (CEasyCamClient* )param;

    
    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"p2pConnectThread");
#endif
#endif
    
    //LOGD("Attach connect thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("Connect thread env attached");
    
    /*
     bEnableLanSearch:
     
     Bit 0 [LanSearch] , 0: Disable Lan search, 1: Enable Lan Search
     Bit 1~4 [P2P Try time]:
     0 (0b0000): 5 second (default)
     15 (0b1111): 0 second, No P2P trying
     Bit 5 [RelayOff], 0: Relay mode is allowed, 1: No Relay connection
     Bit 6 [ServerRelayOnly], 0: Device Relay is allowed, 1: Only Server relay (if Bit 5 = 1, this value is ignored)
     */
    
    int isNeedLan = 0;
    int isNeedP2p = 1;
    int isNeedRelay = 0;
    CHAR bEnableLanSearch = 0;
    bEnableLanSearch |= isNeedLan;
    if (isNeedP2p)
    {
        bEnableLanSearch |= 0x0;
    }else
    {
        bEnableLanSearch |= 0x1E;
    }
    
    if (isNeedRelay)
    {
        bEnableLanSearch |= (1 << 6);
    }else
    {
        bEnableLanSearch |= (1 << 5);
    }
    
    
    LOGD("Start p2p connect to:%s, connectType:%d, bEnableLanSearch:%8x\n", pThis->mUid, pThis->mConnectType, bEnableLanSearch);
    
CONNECT_REMOTE:
    pThis->mP2pSockHandle = -1;
    pThis->mP2pIsConnecting = true;
    int randomPort = rand()%60000+2048;
    LOGD("Connect by p2p, port:%d", randomPort);
    ret = LINK_ConnectByServer(pThis->mUid, bEnableLanSearch, randomPort);
    pThis->mP2pIsConnecting = false;
    if( ret < 0 )
    {
        // Connect fail
        LOGE("P2p Connect to:%s fail, ret:%d, error:%s \n", pThis->mUid, ret, getP2PErrorCodeInfo(ret) );
        
        if( pThis->mNeedBreakConnect || (ret==ERROR_LINK_USER_CONNECT_BREAK) )
        {
			//Don't callback if interrupted by user
		}
        else
        {
            LOGI("Remote is offline, retry...");
            goto CONNECT_REMOTE;
        }
        
        pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
        DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
        pthread_mutex_unlock( &pThis->mJNIEnvMutex );
        
        LOGD("p2pConnectThread quit by self");
        return NULL;
    }
    else
    {
        st_LINK_Session Sinfo;
        if(LINK_Check(ret, &Sinfo) == ERROR_LINK_SUCCESSFUL)
        {
            LOGD("P2p connect to remote success, mode:%s, cost time:%d, localAddr:(%s:%d), remoteAddr:(%s:%d)",
                 ((Sinfo.bMode ==0)? "P2P":"RLY"), Sinfo.ConnectTime,
                 inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port),
                 inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
        }
        
        int isGetAck = 0;
        char recvBuf[16] = {0};
        int recvSize = 3;
        int recvTimeIndex = 0;
        int recvDataIndex = 0;
        LOGD("P2p connect to remote success, start get ACK.");
        while (!pThis->mNeedBreakConnect)
        {
            recvSize = 3;
            recvSize -= recvDataIndex;
            LINK_Read(ret, 0, recvBuf+recvDataIndex, &recvSize, 10);        //10ms
            
            recvDataIndex += recvSize;
            if (recvDataIndex == 3)
            {
                if (0 == strcmp(recvBuf, "ACK"))
                {
                    isGetAck = 1;
                    break;
                }
            }
            
            recvTimeIndex ++;
            if (recvTimeIndex >= 500)       // 5s
            {
                LOGI("p2p recv ack timeout, retry...");
                goto CONNECT_REMOTE;
            }
            
        }
        
        LOGD("p2p quit get ack while, isGetAck:%d", isGetAck);
        
        if (isGetAck)
        {
            LOGD("p2p isGetAck:%d, Relay sock:%d", isGetAck, ret);
            pThis->mP2pSockHandle = ret;
            pThis->mP2pIsConnectSuccess = true;
        } else{
            LINK_ForceClose(ret);
        }
        
    }
    
    
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    
    return NULL;
}

void* CEasyCamClient::relayConnectThread(void* param)
{
    int ret;
    CEasyCamClient* pThis = (CEasyCamClient* )param;

    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"relayConnectThread");
#endif
#endif
    
    //LOGD("Attach connect thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("Connect thread env attached");
    
    /*
     bEnableLanSearch:
     
     Bit 0 [LanSearch] , 0: Disable Lan search, 1: Enable Lan Search
     Bit 1~4 [P2P Try time]:
     0 (0b0000): 5 second (default)
     15 (0b1111): 0 second, No P2P trying
     Bit 5 [RelayOff], 0: Relay mode is allowed, 1: No Relay connection
     Bit 6 [ServerRelayOnly], 0: Device Relay is allowed, 1: Only Server relay (if Bit 5 = 1, this value is ignored)
     */
    
    int isNeedLan = 0;
    int isNeedP2p = 0;
    int isNeedRelay = 1;
    CHAR bEnableLanSearch = 0;
    bEnableLanSearch |= isNeedLan;
    if (isNeedP2p)
    {
        bEnableLanSearch |= 0x0;
    }else
    {
        bEnableLanSearch |= 0x1E;
    }
    
    if (isNeedRelay)
    {
        bEnableLanSearch |= (1 << 6);
    }else
    {
        bEnableLanSearch |= (1 << 5);
    }
    
    LOGD("Start relay connect to:%s, connectType:%d, bEnableLanSearch:%8x\n", pThis->mUid, pThis->mConnectType, bEnableLanSearch);
    
CONNECT_REMOTE:
    pThis->mRelaySockHandle = -1;
    pThis->mRelayIsConnecting = true;
    int randomPort = rand()%60000+2048;
    LOGD("Connect by relay, port:%d", randomPort);
    ret = LINK_ConnectByServer(pThis->mUid, bEnableLanSearch, randomPort);
    pThis->mRelayIsConnecting = false;
    if( ret < 0 )
    {
        // Connect fail
        LOGE("Relay connect to:%s fail, ret:%d, error:%s \n", pThis->mUid, ret, getP2PErrorCodeInfo(ret) );
        
        if( pThis->mNeedBreakConnect || (ret==ERROR_LINK_USER_CONNECT_BREAK) )
        {
			//Don't callback if interrupted by user
		}
        else
        {
            LOGI("Remote is offline, retry...");
            goto CONNECT_REMOTE;
        }
        
        pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
        DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
        pthread_mutex_unlock( &pThis->mJNIEnvMutex );
        
        LOGD("relayConnectThread quit by self");
        return NULL;
    }
    else
    {
        st_LINK_Session Sinfo;
        if(LINK_Check(ret, &Sinfo) == ERROR_LINK_SUCCESSFUL)
        {
            LOGD("Relay connect to remote success, mode:%s, cost time:%d, localAddr:(%s:%d), remoteAddr:(%s:%d)",
                 ((Sinfo.bMode ==0)? "P2P":"RLY"), Sinfo.ConnectTime,
                 inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port),
                 inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
        }
        
        int isGetAck = 0;
        char recvBuf[16] = {0};
        int recvSize = 3;
        int recvTimeIndex = 0;
        int recvDataIndex = 0;
        LOGD("Relay connect to remote success, start get ACK.");
        while (!pThis->mNeedBreakConnect)
        {
            recvSize = 3;
            recvSize -= recvDataIndex;
            LINK_Read(ret, 0, recvBuf+recvDataIndex, &recvSize, 10);        //10ms
            
            recvDataIndex += recvSize;
            if (recvDataIndex == 3)
            {
                if (0 == strcmp(recvBuf, "ACK"))
                {
                    isGetAck = 1;
                    break;
                }
            }
            
            recvTimeIndex ++;
            if (recvTimeIndex >= 500)  // 5s
            {
                LOGI("relay recv ack timeout, retry...");
                goto CONNECT_REMOTE;
            }
            
        }
        
        LOGD("relay quit get ack while, isGetAck:%d", isGetAck);

        if (isGetAck)
        {
            LOGD("relay isGetAck:%d, Relay sock:%d", isGetAck, ret);
            pThis->mRelaySockHandle = ret;
            pThis->mRelayIsConnectSuccess = true;
        }else{
            LINK_ForceClose(ret);
        }
    }
    
    
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    
    return NULL;
}


void* CEasyCamClient::connectTimeThread(void* param)
{
    CEasyCamClient* pThis = (CEasyCamClient* )param;

    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"connectTimeThread");
#endif
#endif

    
    pThis->mConnectTimeRuning = true;
    
    int sleepTime = 5000;
    int counts = 0;
    int sumCounts = pThis->mConnectTimeoutValue * 1000 * 1000 / sleepTime;
    
    LOGD("connectTimeThread sumCounts:%d", sumCounts);
    
    //LOGD("Attach connect thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("Connect thread env attached");
    
    int lanSuccessCounts = -1;
    int p2pSuccessCounts = -1;
    int relaySuccessCounts = -1;
    
    int sockHandle = -1;
    
    while (!(pThis->mNeedBreakConnect))
    {
        if (pThis->mLanIsConnectSuccess)
        {
            sockHandle = pThis->mLanSockHandle;
            
            lanSuccessCounts = counts;
            LOGD("lan connect success, lanSuccessCounts:%d", lanSuccessCounts);
            break;
        }
        
        if (pThis->mP2pIsConnectSuccess)
        {
            sockHandle = pThis->mP2pSockHandle;
            p2pSuccessCounts = counts;
            LOGD("p2p connect success, p2pSuccessCounts:%d", p2pSuccessCounts);
            break;
        }
        
        if (pThis->mRelayIsConnectSuccess)
        {
//            if (-1 == relaySuccessCounts)
//            {
//                sockHandle = pThis->mRelaySockHandle;
//                relaySuccessCounts = counts;
//                LOGD("relay connect success, wait 2s for P2P ready");
//            }else
//            {
//                if ( (counts-relaySuccessCounts) >= 20 )
//                {
//                    LOGD("Connect to remote success, use RELAY");
//                    break;
//                }
//            }
            
            sockHandle = pThis->mRelaySockHandle;
            relaySuccessCounts = counts;
            LOGD("relay connect success, relaySuccessCounts:%d", relaySuccessCounts);
            break;
        }
        
        if (counts >= sumCounts)
        {
            if (pThis->mLanIsConnectSuccess)
            {
                LOGD("Connect to remote time out but lan success, use lan");
                sockHandle = pThis->mLanSockHandle;
            }else if(pThis->mP2pIsConnectSuccess)
            {
                LOGD("Connect to remote time out but p2p success, use p2p");
                sockHandle = pThis->mP2pSockHandle;
            }else if(pThis->mRelayIsConnectSuccess)
            {
                LOGD("Connect to remote time out but relay success, use relay");
                sockHandle = pThis->mRelaySockHandle;
            }else
            {
                LOGD("Connect to remote fail, time out");
                sockHandle = -1;
            }
            
            break;
        }
        
        usleep(sleepTime);     //5ms
        counts++;
    }
    
    if( sockHandle!=-1 && sockHandle == pThis->mLanSockHandle )
    {
        LOGD("Connect by LAN");
    }
    else if( sockHandle!=-1 && sockHandle == pThis->mP2pSockHandle )
    {
        LOGD("Connect by P2P");
    }
    else if( sockHandle!=-1 && sockHandle == pThis->mRelaySockHandle )
    {
        LOGD("Connect by Relay");
    }
    
    pThis->mNeedBreakConnect = true;
    LINK_Connect_Break();
    
    pThis->mBroadcastThreadRun = false;
    if ( !PTHREAD_IS_NULL(pThis->mBroadcastThread) )
    {
        pthread_join(pThis->mBroadcastThread, NULL);
		INIT_THREAD_ID(pThis->mBroadcastThread);
        LOGD("broadcast thread joined");
    }
    
    LOGD("Start join connect threads");
    pthread_join(pThis->mLanConnectThread, NULL);
	INIT_THREAD_ID(pThis->mLanConnectThread);
    LOGD("Lan connect thread joined");
    pthread_join(pThis->mP2pConnectThread, NULL);
	INIT_THREAD_ID(pThis->mP2pConnectThread);
    LOGD("P2P connect thread joined");
    pthread_join(pThis->mRelayConnectThread, NULL);
	INIT_THREAD_ID(pThis->mRelayConnectThread);
    LOGD("Relay connect thread joined");
    
    if (pThis->mLanSockHandle != -1 && pThis->mLanSockHandle != sockHandle)
    {
        LOGD("mLanSockHandle:%d, close this sock handle", pThis->mLanSockHandle);
        LINK_ForceClose(pThis->mLanSockHandle);
        pThis->mLanSockHandle = -1;
    }
    
    if (pThis->mP2pSockHandle != -1 && pThis->mP2pSockHandle != sockHandle)
    {
        LOGD("mP2pSockHandle:%d, close this sock handle", pThis->mP2pSockHandle);
        LINK_ForceClose(pThis->mP2pSockHandle);
        pThis->mP2pSockHandle = -1;
    }
    
    if (pThis->mRelaySockHandle != -1 && pThis->mRelaySockHandle != sockHandle)
    {
        LOGD("mRelaySockHandle:%d, close this sock handle", pThis->mRelaySockHandle);
        LINK_ForceClose(pThis->mRelaySockHandle);
        pThis->mRelaySockHandle = -1;
    }

    pThis->mLanIsConnectSuccess = false;
    pThis->mP2pIsConnectSuccess = false;
    pThis->mRelayIsConnectSuccess = false;
    
    if (sockHandle != -1)
    {
        LOGD("Connect success");
        pThis->mSockHandle = sockHandle;
        pThis->sendLoginCmd(sockHandle);
        
        pThis->mAVRecvThreadRun = true;
        pthread_create( &pThis->mAVRecvThread, NULL, avRecvThread, pThis);
        pThis->mHasAVRecvThreadStarted = true;
        
//        pthread_create( &pThis->mAVRecvThread, NULL, avUdpRecvThread, pThis);   //by hwh, just debug
        
        pThis->mCmdRecvThreadRun = true;
        pthread_create( &pThis->mCmdRecvThread, NULL, cmdRecvThread, pThis );
        pThis->mHasCmdRecvThreadStarted = true;
    }else
    {
        LOGD("Connect fail");
        if( pThis->mInitInfo.lpLoginResult != NULL )
        {
            pThis->mInitInfo.lpLoginResult(pThis->mSessionHandle, LOGIN_RESULT_CONNECT_FAIL,
                                           pThis->mLoginSeq, 0, 0, 0);
        }
    }
    
    pThis->mConnectTimeRuning = false;
    
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self() );
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    
    return NULL;
}


int CEasyCamClient::logOut(void)
{
    mBroadcastThreadRun = false;
    LOGD("Start join connect threads");
	if( !PTHREAD_IS_NULL(mBroadcastThread) )
    {
        pthread_join(mBroadcastThread, NULL);
		INIT_THREAD_ID( mBroadcastThread );
        LOGD("broadcast thread joined");
    }

    LOGD("logOut, uid:%s, sockHandle:%d", mUid, mSockHandle);
//    if( mLanIsConnecting || mP2pIsConnecting || mRelayIsConnecting || mConnectTimeRuning )  //判断各个连接线程是否还在运行
//    {
//        LOGD("PPCS_Connect_Break");
//        mNeedBreakConnect = true;
//        PPCS_Connect_Break();
//    }
    LOGD("PPCS_Connect_Break");
    mNeedBreakConnect = true;
    LINK_Connect_Break();
    
    if( !PTHREAD_EQUAL(pthread_self(), mLanConnectThread) )
    {
        if( !PTHREAD_IS_NULL(mLanConnectThread) )
        {
            LOGD("Start join lan connect thread");
            pthread_join( mLanConnectThread, NULL );
			INIT_THREAD_ID(mLanConnectThread);
            LOGD("P2p connect thread joined");
        }
    }
    else
    {
        LOGI("Connect fail in p2pConnectThread, no need to join p2pConnectThread");
    }
    
    if( !PTHREAD_EQUAL(pthread_self(), mP2pConnectThread) )
    {
        if( !PTHREAD_IS_NULL(mP2pConnectThread) )
        {
            LOGD("Start join p2p connect thread");
            pthread_join( mP2pConnectThread, NULL );
			INIT_THREAD_ID(mP2pConnectThread);
            LOGD("P2p connect thread joined");
        }
    }
    else
    {
        LOGI("Connect fail in p2pConnectThread, no need to join p2pConnectThread");
    }
    
    if( !PTHREAD_EQUAL(pthread_self(), mRelayConnectThread) )
    {
        if( !PTHREAD_IS_NULL(mRelayConnectThread) )
        {
            LOGD("Start join relay connect thread");
            pthread_join( mRelayConnectThread, NULL );
            LOGD("Relay connect thread joined");
			INIT_THREAD_ID(mRelayConnectThread);
        }
    }
    else
    {
        LOGI("Connect fail in relayConnectThread, no need to join relayConnectThread");
    }
    
    if( !PTHREAD_EQUAL(pthread_self(), mConnectTimeThread) )
    {
		if ( !PTHREAD_IS_NULL(mConnectTimeThread) )
		{
			pthread_join(mConnectTimeThread, NULL);
			INIT_THREAD_ID(mConnectTimeThread);
		}
    }
    
    LOGD("PPCS_Connect_Break");
    mNeedBreakConnect = true;
    LINK_Connect_Break();
    
    if( mHasCmdRecvThreadStarted )
    {
        LOGD("Start join cmd recv thread");
        mCmdRecvThreadRun = false;
        pthread_join( mCmdRecvThread, NULL );
        mHasCmdRecvThreadStarted = false;
        LOGD("cmd recv thread joined");
    }
    
    if( mHasTalkDataSendThreadStarted )
    {
        LOGD("Start join talk data send thread");
        mTalkDataSendThreadRun = false;
        pthread_join(mTalkDataSendThread, NULL);
        mHasTalkDataSendThreadStarted = false;
        LOGD("talk data send thread joined");
    }
    
    if( mHasCmdSendThreadStarted )
    {
        LOGD("Start join cmd send thread");
        mCmdSendThreadRun = false;
        pthread_join(mCmdSendThread, NULL);
        LOGD("cmd send thread joined");
        mHasCmdSendThreadStarted = false;
    }
    
    
    mCmdQueueEnable = false;
    clearCmdQueue();
    
    mTalkDataQueueEnable = false;
    clearTalkDataQueue();
    
    if( mHasAVRecvThreadStarted )
    {
        mAVRecvThreadRun = false;
        LOGD("Start join AV recv thread");
        pthread_join( mAVRecvThread, NULL );
        mHasAVRecvThreadStarted = false;
        LOGD("AV recv thread joined");
    }
    
    closeConnection();
    
    LOGD("CEasyCamClient logout complete");
    
    return 0;

}

int CEasyCamClient::sendCommand(char* command, int seq)
{
	char* pCmdMsg = NULL;
	BUILD_CMD_MSG(pCmdMsg, command, seq );
	if( pCmdMsg == NULL )
	{
		LOGE("BUILD_CMD_MSG fail");
		return -1;
	}
	if( addCmdToQueue( pCmdMsg ) != 0 )
	{
		LOGE("addCmdToQueue fail");
		return -1;
	}

    cJSON * pJson = cJSON_Parse(command);
    cJSON * pItem = NULL;
    if( pJson != NULL )
    {
        pItem = cJSON_GetObjectItem(pJson, "cmdId");
        if( pItem != NULL )
        {
            int cmdId = pItem->valueint;
            //int EC_CMD_ID_START_PLAY_EVENT_RECORD = 0x302;
            //int EC_CMD_ID_START_PLAY_RECORD = 0x303;
            //int EC_CMD_ID_STOP_PLAY_RECORD = 0x304;
            if( (cmdId == 0x303) || (cmdId == 0x302) ){
                LOGD("Start play cmd, seq:%d", seq);

                if( mHasPBRecvThreadStarted )
                {
                    mPBRecvThreadRun = false;
                    LOGD("Start join Playback thread");
                    pthread_join( mPBRecvThread, NULL );
                    LOGD("Playback thread joined");
                    mHasPBRecvThreadStarted = false;
                }

                mPBRecvThreadRun = true;
                PlaybackThreadContextInfo *pContextInfo = new PlaybackThreadContextInfo();
                pContextInfo->pThis = this;
                pContextInfo->cmdSeq = seq;
                pthread_create( &mPBRecvThread, NULL, pbRecvThread, pContextInfo );
                mHasPBRecvThreadStarted = true;
            }else if( cmdId == 0x304 ){
                LOGD("Stop play cmd");

                if( mHasPBRecvThreadStarted )
                {
                    mPBRecvThreadRun = false;
                    LOGD("Start join Playback thread");
                    pthread_join( mPBRecvThread, NULL );
                    LOGD("Playback thread joined");
                    mHasPBRecvThreadStarted = false;
                }
            }else if( cmdId == 0x506 ){
                if( mHasFileDownloadThreadStarted )
                {
                    mFileDownloadThreadRun = false;
                    LOGD("Start join file download thread");
                    pthread_join( mFileDownloadThread, NULL );
                    LOGD("File download thread joined");
                    mHasFileDownloadThreadStarted = false;
                }

                mFileDownloadThreadRun = true;
                FileDownloadThreadContextInfo *pContextInfo = new FileDownloadThreadContextInfo();
                pContextInfo->pThis = this;
                pContextInfo->cmdSeq = seq;
                pthread_create( &mFileDownloadThread, NULL, fileDownloadRecvThread, pContextInfo );
                mHasFileDownloadThreadStarted = true;
            }else if( cmdId == 0x507 ){
                if( mHasFileDownloadThreadStarted )
                {
                    mFileDownloadThreadRun = false;
                    LOGD("Start join file download thread");
                    pthread_join( mFileDownloadThread, NULL );
                    LOGD("File download thread joined");
                    mHasFileDownloadThreadStarted = false;
                }
            }else if( cmdId == 0x508 ){
                if( msHasPIRDataRecvThreadStarted )
                {
                    mIsPIRDataRecvThreadRun = false;
                    LOGD("Start join PIR data recv thread");
                    pthread_join( mPIRDataRecvThread, NULL );
                    LOGD("PIR data recv thread joined");
                    msHasPIRDataRecvThreadStarted = false;
                }

                mIsPIRDataRecvThreadRun = true;
                pthread_create( &mPIRDataRecvThread, NULL, PIRDataRecvThread, this );
                msHasPIRDataRecvThreadStarted = true;
            }else if( cmdId == 0x509 ){
                if( msHasPIRDataRecvThreadStarted )
                {
                    mIsPIRDataRecvThreadRun = false;
                    LOGD("Start join PIR data recv thread");
                    pthread_join( mPIRDataRecvThread, NULL );
                    LOGD("PIR data recv thread joined");
                    msHasPIRDataRecvThreadStarted = false;
                }
            }
        }
    }

	LOGI("SendCommand:%s", command);

	return 0;
}

int CEasyCamClient::sendTalkData(char* data, int dataLen, int payloadType, int seq)
{
    char* pTalkPack = NULL;

    /*
     * This function processes a 10/20ms frame and adjusts (normalizes) the gain
     * both analog and digitally. The gain adjustments are done only during
     * active periods of speech. The input speech length can be either 10ms or
     * 20ms and the output is of the same length. The length of the speech
     * vectors must be given in samples (80/160 when FS=8000, and 160/320 when
     * FS=16000 or FS=32000). The echo parameter can be used to ensure the AGC will
     * not adjust upward in the presence of echo.
     *
     * This function should be called after processing the near-end microphone
     * signal, in any case after any echo cancellation.
     *
     * Input:
     *      - agcInst           : AGC instance
     *      - inNear            : Near-end input speech vector (10 or 20 ms) for
     *                            L band
     *      - inNear_H          : Near-end input speech vector (10 or 20 ms) for
     *                            H band
     *      - samples           : Number of samples in input/output vector
     *      - inMicLevel        : Current microphone volume level
     *      - echo              : Set to 0 if the signal passed to add_mic is
     *                            almost certainly free of echo; otherwise set
     *                            to 1. If you have no information regarding echo
     *                            set to 0.
     *
     * Output:
     *      - outMicLevel       : Adjusted microphone volume level
     *      - out               : Gain-adjusted near-end speech vector (L band)
     *                          : May be the same vector as the input.
     *      - out_H             : Gain-adjusted near-end speech vector (H band)
     *      - saturationWarning : A returned value of 1 indicates a saturation event
     *                            has occurred and the volume cannot be further
     *                            reduced. Otherwise will be set to 0.
     *
     * Return value:
     *                          :  0 - Normal operation.
     *                          : -1 - Error
     */
    int inMicLevel  = mMicLevelOut;
    int outMicLevel = 0;
    int frameSize = dataLen/2;
    uint8_t saturationWarning;
    int nAgcRet = WebRtcAgc_Process(mAgcHandle, (short *)data, NULL, frameSize,
            (short *)mTalkAgcOutBuf, NULL, inMicLevel, &outMicLevel, 0, &saturationWarning);
    if (nAgcRet != 0)
    {
        LOGE("Failed in WebRtcAgc_Process, ret:%d", nAgcRet);
    }
    //LOGD("saturationWarning:%d,outMicLevel:%d", saturationWarning, outMicLevel);
    mMicLevelIn = outMicLevel;

    payloadType = 0x11; // ADPCM
    adpcm_encoder( (short *)mTalkAgcOutBuf, mTalkAdpcmBuf, dataLen/2, &mAdpcmEncState);
    BUILD_TALK_PACK(pTalkPack, mTalkAdpcmBuf, (dataLen/4), payloadType, seq);
    
//    BUILD_TALK_PACK(pTalkPack, data, dataLen, payloadType, seq);
    if( pTalkPack == NULL )
    {
        LOGE("Build talk pack fail");
        return -1;
    }

    //LOGD("Send talk data, len:%d", dataLen);
    if( addTalkDataToQueue(pTalkPack) != 0 )
    {
        LOGE("addTalkDataToQueue fail");
        return -1;
    }

    return 0;
}

unsigned int CEasyCamClient::getToken()
{
    return mToken;
}

void CEasyCamClient::joinThreads(void)
{
	mCmdQueueEnable = false;
	clearCmdQueue();

    if( mHasCmdRecvThreadStarted )
    {
        LOGD("Start join cmd recv thread");
        mCmdRecvThreadRun = false;
        pthread_join( mCmdRecvThread, NULL );
        mHasCmdRecvThreadStarted = false;
        LOGD("cmd recv thread joined");
    }

    if( mHasAVRecvThreadStarted )
    {
        mAVRecvThreadRun = false;
        LOGD("Start join AV recv thread");
        pthread_join( mAVRecvThread, NULL );
        mHasAVRecvThreadStarted = false;
        LOGD("AV recv thread joined");
    }

    if( mHasPBRecvThreadStarted )
    {
        mPBRecvThreadRun = false;
        LOGD("Start join playback thread");
        pthread_join(mPBRecvThread, NULL);
        LOGD("Playback thread joined");
        mHasPBRecvThreadStarted = false;
    }

    if( mHasFileDownloadThreadStarted )
    {
        mFileDownloadThreadRun = false;
        LOGD("Start join file download thread");
        pthread_join(mFileDownloadThread, NULL);
        LOGD("File download thread joined");
        mHasFileDownloadThreadStarted = false;
    }
}

void CEasyCamClient::closeConnection(void)
{
	LOGD("Close session, handle:%d", mSockHandle);
	if( mSockHandle >= 0 )
	{
	    LOGD("Close handle:%d", mSockHandle);
	    LINK_ForceClose( mSockHandle );
        mSockHandle = -1;
	}
}

int CEasyCamClient::addCmdToQueue(void* cmd)
{
	if( cmd == NULL )
		return -1;

	int ret = 0;

	if( !mCmdQueueEnable )
	{
		LOGE("Cmd queue is disabled now");
		return -1;
	}

	pthread_mutex_lock( &mCmdQueueMutex );
	if( mCmdQueue.size() >= MAX_CMD_QUEUE_SIZE )
	{
		// Cmd queue full
		LOGI("Cmd queue is full");
		ret = -1;
	}
	else
	{
		mCmdQueue.push_back( cmd );
	}
	pthread_mutex_unlock( &mCmdQueueMutex );

	return ret;
}

void* CEasyCamClient::getCmdFromQueue(void)
{
	void * cmd = NULL;

	pthread_mutex_lock( &mCmdQueueMutex );
	if( mCmdQueue.size() == 0 )
	{
		cmd = NULL;
	}
	else
	{
		cmd = mCmdQueue.front();
		mCmdQueue.pop_front();
	}
	pthread_mutex_unlock( &mCmdQueueMutex );

	return cmd;
}

void CEasyCamClient::clearCmdQueue(void)
{
	std::list<void* >::iterator it;

	pthread_mutex_lock( &mCmdQueueMutex );
	for( it = mCmdQueue.begin(); it != mCmdQueue.end(); it++ )
	{
		if( (*it) != NULL )
		{
			pthread_mutex_lock( &mCmdMsgMutex );
			EasyCamMsgHead* pMsg = (EasyCamMsgHead* )(*it);
			if( pMsg != NULL )
			{
				delete (pMsg);
			}
			pthread_mutex_unlock( &mCmdMsgMutex );
		}
	}
	mCmdQueue.clear();
	pthread_mutex_unlock( &mCmdQueueMutex );
}

int CEasyCamClient::addTalkDataToQueue(void* data)
{
	if( data == NULL )
		return -1;

	int ret = 0;

	if( !mTalkDataQueueEnable )
	{
		LOGE("talk data queue is disabled now");
		return -1;
	}

	pthread_mutex_lock( &mTalkDataQueueMutex );
	if( mTalkDataQueue.size() >= MAX_TALK_DATA_QUEUE_SIZE )
	{
		// Talk data queue full
		LOGI("Talk data queue is full");
		ret = -1;
	}
	else
	{
		mTalkDataQueue.push_back( data );
	}
	pthread_mutex_unlock( &mTalkDataQueueMutex );

	return ret;
}

void* CEasyCamClient::getTalkDataFromQueue(void)
{
	void * data = NULL;

	pthread_mutex_lock( &mTalkDataQueueMutex );
	if( mTalkDataQueue.size() == 0 )
	{
		data = NULL;
	}
	else
	{
		data = mTalkDataQueue.front();
		mTalkDataQueue.pop_front();
	}
	pthread_mutex_unlock( &mTalkDataQueueMutex );

	return data;
}

void CEasyCamClient::clearTalkDataQueue(void)
{
	std::list<void* >::iterator it;

	pthread_mutex_lock( &mTalkDataQueueMutex );
	for( it = mTalkDataQueue.begin(); it != mTalkDataQueue.end(); it++ )
	{
		if( (*it) != NULL )
		{
			FrameHeadInfo* pTalkFrame = (FrameHeadInfo* )(*it);
			if( pTalkFrame != NULL )
			{
				delete (pTalkFrame);
			}
		}
	}
	mTalkDataQueue.clear();
	pthread_mutex_unlock( &mTalkDataQueueMutex );
}

int CEasyCamClient::sendLoginCmd(int sessionHandle)
{
    // Send login command
    char* pLogInCmd = NULL;
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;
    pJsonRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", EC_CMD_ID_LOGIN);
    cJSON_AddStringToObject(pJsonRoot, "usrName", mUsrAccount);
    cJSON_AddStringToObject(pJsonRoot, "password", mUsrPassword);
    cJSON_AddNumberToObject(pJsonRoot, "needVideo", mLoginNeedVideo);
    cJSON_AddNumberToObject(pJsonRoot, "needAudio", mLoginNeedAudio);
    jsonStr = cJSON_Print(pJsonRoot);
    cJSON_Minify(jsonStr);
    int seq = mLoginSeq;
    BUILD_CMD_MSG(pLogInCmd, jsonStr, seq );
    LOGD("Login cmd:%s", jsonStr);
    cJSON_Delete(pJsonRoot);

    if( addCmdToQueue( pLogInCmd ) != 0 )
    {
        LOGE("Add cmd to queue fail");
        if( mInitInfo.lpLoginResult != NULL )
        {
            mInitInfo.lpLoginResult(sessionHandle, LOGIN_RESULT_FAIL_UNKOWN, mLoginSeq, 0, 0, 0);
        }
    }

    return 0;
}

void* CEasyCamClient::broadcastThread(void* param)
{

#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"broadcastThread");
#endif
#endif
    CEasyCamClient* pThis = (CEasyCamClient* )param;
    pThis->mBroadcastThreadRun = true;

    int brdcFdRet;
    int brdcFd;
    if((brdcFd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        //printf("socket fail\n");
        return NULL;
    }else
    {
#ifdef _WIN32
		char optval = 1;
#else
        int optval = 1;
#endif
        brdcFdRet = setsockopt(brdcFd, SOL_SOCKET, SO_BROADCAST, &optval,sizeof(int));
        if(brdcFdRet!=0)
        {
            //printf("setsockopt SO_BROADCAST error:%d, %s\n", errno, strerror(errno));
			SOCKET_CLOSE(brdcFd);
            return NULL;
        }
        brdcFdRet = setsockopt(brdcFd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(int));
        if(brdcFdRet!=0)
        {
            //printf("setsockopt SO_REUSEADDR error:%d, %s\n", errno, strerror(errno));
			SOCKET_CLOSE(brdcFd);
            return NULL;
        }
        
        struct sockaddr_in theirAddr;
        memset(&theirAddr, 0, sizeof(struct sockaddr_in));
        theirAddr.sin_family = AF_INET;
        theirAddr.sin_addr.s_addr = inet_addr(pThis->mBroadcastAddr);
        theirAddr.sin_port = htons(2888);
        //printf("broadcastThread, pThis->mBroadcastAddr:%s\n", pThis->mBroadcastAddr);
        
        while (pThis->mBroadcastThreadRun)
        {
            int sendBytes;
            if((sendBytes = sendto(brdcFd, pThis->mUid, strlen(pThis->mUid), 0, (struct sockaddr *)&theirAddr, sizeof(struct sockaddr))) == -1)
            {
                
               // printf("sendto fail, errno=%d\n", errno);
               // printf("errno str:%s\n", strerror(errno));
            }else
            {
               // printf("sendto ok, msg:%s, msg len:%lu\n", pThis->mUid, strlen(pThis->mUid));
            }
            usleep(100*1000);   //100ms
        }
		SOCKET_CLOSE(brdcFd);
        
    }

    return NULL;
}

void CEasyCamClient::handleCmdLoginResp(char* data, int seq)
{
	int logInErrorCode = LOGIN_RESULT_SUCCESS; // success
	int logInResult = -255;

	cJSON * pJson = cJSON_Parse(data);
	cJSON * pItem = NULL;
	pItem = cJSON_GetObjectItem(pJson, "result");
	if( pItem != NULL )
	{
		logInResult = pItem->valueint;
	}
	if( logInResult == 0 ) {
		logInErrorCode = LOGIN_RESULT_SUCCESS;
	}else if( logInResult == -1 ){
		logInErrorCode = LOGIN_RESULT_AUTH_FAIL;
	}else if( logInResult == -2 ){
		logInErrorCode = LOGIN_RESULT_EXCEED_MAX_CONNECTION;
	}

	// get notification token
	unsigned int notificationToken = 0;
	pItem = cJSON_GetObjectItem(pJson, "event_channel");           // used for message push
    if( pItem != NULL )
    {
        notificationToken = pItem->valueint;
    }
    
    pItem = cJSON_GetObjectItem(pJson, "token");           
    if( pItem != NULL )
    {
        mToken = pItem->valueint;
    }

    unsigned int batPercent = 0;
    pItem = cJSON_GetObjectItem(pJson, "bat_percent");
    if( pItem != NULL )
    {
        batPercent = pItem->valueint;
    }

    unsigned int isCharging = 0;
    pItem = cJSON_GetObjectItem(pJson, "is_charging");
    if( pItem != NULL )
    {
        isCharging = pItem->valueint;
    }
    
//    unsigned int isVolSetSupport = 0;
//    pItem = cJSON_GetObjectItem(pJson, "isVolSetSupport");
//    if( pItem != NULL )
//    {
//        isVolSetSupport = pItem->valueint;
//    }
    

	if( mInitInfo.lpLoginResult != NULL )
	{
		mInitInfo.lpLoginResult(mSessionHandle, logInErrorCode, seq, notificationToken, isCharging, batPercent);
	}
}

void CEasyCamClient::handleCmdResp(char* data, int seq)
{
	if( mInitInfo.lpCmdResult )
	{
		mInitInfo.lpCmdResult(mSessionHandle, data, seq);
	}
}

int CEasyCamClient::handleMsg(int fd, char* data, int len)
{
	EasyCamMsgHead* pMsgHead = (EasyCamMsgHead* )data;
	//LOGD("handleMsg, msgId:%d", pMsgHead->msgId);
	if( pMsgHead->msgId == EC_MSG_ID_CMD_RESP )
	{
		int cmdId = 0;
		cJSON * pJson = cJSON_Parse(pMsgHead->data);
		cJSON * pItem = NULL;
		if( pJson == NULL )
		{
			LOGE("Parse cmd response fail");
			return -1;
		}
		pItem = cJSON_GetObjectItem(pJson, "cmdId");
		if( pItem == NULL )
		{
			LOGE("get cmdId fail");
			return -1;
		}
		cmdId = pItem->valueint;
		if( cmdId == EC_CMD_ID_LOGIN )
		{
			LOGD("Got response of login:%s", cJSON_Print(pJson));
			handleCmdLoginResp(pMsgHead->data, pMsgHead->seq);
		}
		else
		{
			handleCmdResp(pMsgHead->data, pMsgHead->seq);
		}
		cJSON_Delete(pJson);

		return 0;
	}
	else if( pMsgHead->msgId == EC_MSG_ID_AV_DATA )
	{
		// Recv av data
		FrameHeadInfo* pFrame = (FrameHeadInfo* )pMsgHead->data;
		//LOGD("videoLen:%d, audioLen:%d, fd:%d", pFrame->videoLen, pFrame->audioLen, fd);
		if( pFrame->audioLen > 0 )
		{
			if( mInitInfo.lpAudio_RecvData != NULL )
			{
				int audioPayload = 0;
				int audioLen = 0;
				char* pAudioBuf = pFrame->data+pFrame->videoLen;
				if( pFrame->audio_format == EASYCAM_AUDIO_FORMAT_PCM ){
					audioPayload = PAYLOAD_TYPE_AUDIO_PCM;
					audioLen = pFrame->audioLen;
					pAudioBuf = pFrame->data+pFrame->videoLen;
				}else if( pFrame->audio_format == EASYCAM_AUDIO_FORMAT_ADPCM ){
				    adpcm_decoder((char*)pFrame->data+pFrame->videoLen, (short *)mPCMBuf,
				        pFrame->audioLen*2, &mAdpcmDecState);
				    pAudioBuf = mPCMBuf;
				    audioLen = pFrame->audioLen*4;
					audioPayload = PAYLOAD_TYPE_AUDIO_PCM;
				}

				// T.B.D 16k sample rate?
				if( mNsHandle )
				{
				    if( mNsBufSize+audioLen < 10240 )
				    {
				        memcpy( mNsBuf+mNsBufSize, pAudioBuf, audioLen );
                        mNsBufSize += audioLen;

				        int nsPcmSize = 0;
                        while( mNsBufSize >= 320 )
                        {
                            // consume one frame
                            // NS
                            WebRtcNs_Process( mNsHandle, (short int* )mNsBuf,
                                    NULL, (short int* )(mNsOutBuf+nsPcmSize), NULL );

                            mNsBufSize -= 320;
                            memmove( mNsBuf, mNsBuf+320, mNsBufSize );

                            nsPcmSize += 320;
                        }

                        mInitInfo.lpAudio_RecvData(mSessionHandle, mNsOutBuf,
                                nsPcmSize, audioPayload, pFrame->timeStamp/1000,
                                (int)(pFrame->seqNo) );
				    }
				}
				else
				{
                    mInitInfo.lpAudio_RecvData(mSessionHandle, pAudioBuf,
                            audioLen, audioPayload, pFrame->timeStamp/1000,
                            (int)(pFrame->seqNo) );
				}
			}
		}

		if( pFrame->videoLen > 0 )
		{
			if( mInitInfo.lpVideo_RecvData != NULL )
			{
				mInitInfo.lpVideo_RecvData(mSessionHandle, pFrame->data, pFrame->videoLen,
						PAYLOAD_TYPE_VIDEO_H264, pFrame->timeStamp/1000,
						(int)(pFrame->seqNo), pFrame->isKeyFrame, pFrame->videoWidth, pFrame->videoHeight,
						pFrame->wifiQuality);
			}
		}

		return 0;
	}
	else if( pMsgHead->msgId == EC_MSG_ID_PB_DATA )
	{
	    FrameHeadInfo* pFrame = (FrameHeadInfo* )pMsgHead->data;
	    /*LOGD("PB data, len:%d, videoLen:%d, audioLen:%d", len, pFrame->videoLen, pFrame->audioLen);*/

	    if( mNeedWaitPlaybackStartFrame )
	    {
	        if( pFrame->frameType == PLAY_RECORD_FRAME_TYPE_START )
	        {
	            mNeedWaitPlaybackStartFrame = false;
	        }
	        else
	        {
	            LOGD("Wait start frame");
	            return 0;
	        }
	    }

	    if( pFrame->frameType == PLAY_RECORD_FRAME_TYPE_END )
	    {
            LOGD("Got end frame");
	        if( mInitInfo.lpPBEnd != NULL )
	        {
	            mInitInfo.lpPBEnd(mSessionHandle, pFrame->pbSessionNo);
	        }
	        return 0;
	    }

	    if( pFrame->audioLen > 0 )
	    {
	        if( mInitInfo.lpPBAudio_RecvData != NULL )
	        {
	            // timestamp is in ms unit
                mInitInfo.lpPBAudio_RecvData(mSessionHandle, pFrame->data+pFrame->videoLen,
                                        pFrame->audioLen, PAYLOAD_TYPE_AUDIO_PCM, pFrame->timeStamp/1000,
                                        0, pFrame->pbSessionNo );
	        }
	    }

	    if( pFrame->videoLen > 0 )
	    {
	        if( mInitInfo.lpPBVideo_RecvData != NULL )
	        {
	            mInitInfo.lpPBVideo_RecvData(mSessionHandle, pFrame->data, pFrame->videoLen,
            		PAYLOAD_TYPE_VIDEO_H264, pFrame->timeStamp/1000,
            		0, pFrame->isKeyFrame, pFrame->videoWidth, pFrame->videoHeight, pFrame->pbSessionNo);
	        }
	    }
	}
    else if( pMsgHead->msgId == EC_MSG_ID_FILE_DOWNLOAD_DATA )
    {
        //LOGD("File download data, size:%d", pMsgHead->dataLen);
        FrameHeadInfo* pFrame = (FrameHeadInfo* )pMsgHead->data;
        if( pFrame->videoLen > 0 )
        {
            if( mInitInfo.lpFileDownload_RecvData != NULL )
            {
                mInitInfo.lpFileDownload_RecvData(mSessionHandle, pFrame->data, pFrame->videoLen, pFrame->pbSessionNo);
            }
        }
    }
    else if( pMsgHead->msgId == EC_MSG_ID_PIR_DATA ){
        typedef struct tagPIRRawDataInfo{
            long long time;
            short adc;
        }PIRRawDataInfo;
        PIRRawDataInfo* pRawData = (PIRRawDataInfo* )pMsgHead->data;

        LOGD("PIR data, timeMS:%lld, ADC:%d", pRawData->time, pRawData->adc);

        if( mInitInfo.lpPIRData_RecvData != NULL ){
            mInitInfo.lpPIRData_RecvData(mSessionHandle, pRawData->time, pRawData->adc);
        }
    }

	return 0;
}

void* CEasyCamClient::cmdSendThread(void* param)
{
	CEasyCamClient* pThis = (CEasyCamClient* )param;
	void* pCmd = NULL;
	int ret = 0;

#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"cmdSendThread");
#endif
#endif

	LOGD("Enter cmdSendThread, fd:%d", pThis->mSockHandle);

    //LOGD("Attach cmd send thread env, pid:%#X, mJNIEnvMutex:%p", pthread_self(), &pThis->mJNIEnvMutex);
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("cmd send thread env attached");

	while( pThis->mCmdSendThreadRun )
	{
		if( pThis->mSockHandle >= 0 )
		{
			bool isConnectionBroken = false;

			pthread_mutex_lock( &pThis->mCmdMsgMutex );
			pCmd = pThis->getCmdFromQueue();
			if( pCmd == NULL )
			{
				pthread_mutex_unlock( &pThis->mCmdMsgMutex );
				// no command
				usleep(10000);
				continue;
			}
			else
			{
				EasyCamMsgHead* pEasyCamMsg = (EasyCamMsgHead* )pCmd;

				int retryCnt = 0;
				char* pDataWrite = (char* )pCmd;
				int writePos = 0;
				int remainSize = sizeof(EasyCamMsgHead)+pEasyCamMsg->dataLen;
				while( (remainSize>0) && (pThis->mCmdSendThreadRun) )
				{
					if( retryCnt++ < 100 )
					{
						UINT32 ppcsBufSize = 0;
						ret =LINK_Check_Buffer(pThis->mSockHandle, 0,(UINT32 *)&ppcsBufSize, NULL);
						if( (ret == ERROR_LINK_SESSION_CLOSED_TIMEOUT) ||
								(ret == ERROR_LINK_SESSION_CLOSED_REMOTE) ||
								(ret == ERROR_LINK_INVALID_SESSION_HANDLE) )
						{
							LOGE("PPCS_Check_Buffer fail, ret:%d", ret);
							isConnectionBroken = true;
							break;
						}

						if( (ppcsBufSize+remainSize) >= 262144 )//256k
						{
							LOGI("PPCS send buffer size > 256K, size:%d", ppcsBufSize);
							usleep(100000); // 100ms
							continue;
						}

						int writeSize = (remainSize>262144)?262144:remainSize;
						ret = LINK_Write(pThis->mSockHandle, 0, (pDataWrite+writePos), writeSize);
						if( ret > 0 )
						{
							LOGD("Send cmd, len:%d", ret);
							writePos += ret;
							remainSize -= ret;
						}
						else if( ret == 0 )
						{
							usleep(10000);
							continue;
						}
						else
						{
							if( ret == ERROR_LINK_REMOTE_SITE_BUFFER_FULL )
							{
								usleep(100000);
								continue;
							}
							else
							{
								LOGE("PPCS_Write channel 0 fail, ret:%d", ret);
								isConnectionBroken = true;
								break;
							}
						}
					}
					else
					{
						LOGE("Send cmd to remote timeout");
						break;
					}
				}

				/* 发送完成，释放内存*/
				if( pEasyCamMsg != NULL )
				{
					delete(pEasyCamMsg);
				}
			}

			pthread_mutex_unlock( &pThis->mCmdMsgMutex );

			if( isConnectionBroken && !pThis->mHasBrokenDetected)
			{
                pThis->mHasBrokenDetected = true;
				if( pThis->mInitInfo.lpConnectionBroken != NULL )
				{
					pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle, pThis->mSockHandle,
							CONNECTION_BROKEN_ISSUE_SEND_CMD);
				}

				break;
			}
		}
		else
		{
			usleep(10000);
		}
	}

	LOGD("Leave cmdSendThread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
	DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
	pthread_mutex_unlock( &pThis->mJNIEnvMutex );

	return NULL;
}

void* CEasyCamClient::audioSendThread(void* param)
{
    CEasyCamClient* pThis = (CEasyCamClient* )param;
    void* pData = NULL;

    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"audioSendThread");
#endif
#endif

    int ret = 0;

    LOGD("Enter audioSendThread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    while( pThis->mTalkDataSendThreadRun )
    {
        if( pThis->mSockHandle >= 0 )
        {
            bool isConnectionBroken = false;

            pData = pThis->getTalkDataFromQueue();
            if( pData == NULL )
            {
                usleep(5000); // 5ms
                continue;
            }
			else
			{
				EasyCamMsgHead* pEasyCamMsg = (EasyCamMsgHead* )pData;

				int retryCnt = 0;
				char* pDataWrite = (char* )pData;
				int writePos = 0;
				int remainSize = sizeof(EasyCamMsgHead)+pEasyCamMsg->dataLen;

				while( (remainSize>0) && (pThis->mTalkDataSendThreadRun) )
				{
					if( retryCnt++ < 100 )
					{
						UINT32 ppcsBufSize = 0;
						ret = LINK_Check_Buffer(pThis->mSockHandle, 1,(UINT32 *)&ppcsBufSize, NULL);
						if( (ret == ERROR_LINK_SESSION_CLOSED_TIMEOUT) ||
								(ret == ERROR_LINK_SESSION_CLOSED_REMOTE) ||
								(ret == ERROR_LINK_INVALID_SESSION_HANDLE) )
						{
							LOGE("PPCS_Check_Buffer fail, ret:%d", ret);
							isConnectionBroken = true;
							break;
						}

                        //LOGI("PPCS buf size:%d", ppcsBufSize);
						if( (ppcsBufSize+remainSize) >= 262144 )//256k
						{
							LOGI("PPCS send buffer size > 256K, size:%d", ppcsBufSize);
							continue;
						}

						int writeSize = (remainSize>262144)?262144:remainSize;
						ret = LINK_Write(pThis->mSockHandle, 1, (pDataWrite+writePos), writeSize);
						if( ret > 0 )
						{
							writePos += ret;
							remainSize -= ret;
						}
						else if( ret == 0 )
						{
							usleep(10000);
							continue;
						}
						else
						{
							if( ret == ERROR_LINK_REMOTE_SITE_BUFFER_FULL )
							{
								usleep(10000);
								continue;
							}
							else
							{
								LOGE("PPCS_Write channel 1 fail, ret:%d", ret);
								isConnectionBroken = true;
								break;
							}
						}
					}
					else
					{
						LOGE("Send audio data to remote timeout");
						break;
					}
				}

				if( pEasyCamMsg != NULL )
				{
					delete(pEasyCamMsg);
				}
			}
            
            if( isConnectionBroken && !pThis->mHasBrokenDetected)
            {
                pThis->mHasBrokenDetected = true;
                if( pThis->mInitInfo.lpConnectionBroken != NULL )
                {
                    pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle, pThis->mSockHandle,
                                                        CONNECTION_BROKEN_ISSUE_SEND_CMD);
                }
                
                break;
            }
            
        }
        else
        {
            usleep(50000);
        }
    }

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );

    return NULL;
}

void* CEasyCamClient::cmdRecvThread(void* param)
{
	CEasyCamClient* pThis = (CEasyCamClient* )param;

#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"cmdRecvThread");
#endif
#endif

	char *cmdRecvBuf = new char[10240];
	int cmdRecvBufSize = 0;
	char buf[1024];
	int dataSize = 0;
	int ret = 0;

	LOGD("Enter cmdRecvThread, fd:%d", pThis->mSockHandle);

    //LOGD("Attach cmd recv thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("cmd recv thread env attached");

	while( pThis->mCmdRecvThreadRun )
	{
		if( pThis->mSockHandle >= 0 )
		{
			dataSize = 1024;
			ret = LINK_Read(pThis->mSockHandle, 0, buf, &dataSize, 20); // timeout 20ms
            //LOGE("CMD PPCS_Read:%d, session handle:%d, channel:0, dataSize:%d",
            //     ret, pThis->mSockHandle, dataSize);
			if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
			{
				if( dataSize > 0 )
				{
					//LOGD("Recv data, size:%d", dataSize);
					memcpy( cmdRecvBuf+cmdRecvBufSize, buf, dataSize );
					cmdRecvBufSize += dataSize;

					// parse data
					int parsedLen = 0;
					char* pParsePoint = NULL;
					int remainLen = 0;
					int msgDataLen = 0;
					char magic[16] = {'\0'};
					int retryCnt = 0;
					while( 1 )
					{
						pParsePoint = cmdRecvBuf+parsedLen;
						remainLen = cmdRecvBufSize - parsedLen;
						if( (remainLen<=0) || (unsigned int)remainLen < sizeof(EasyCamMsgHead) )
						{
							// no more data to parse
							//LOGI("No more data to parse ,remain:%d", remainLen);
							break;
						}

						EasyCamMsgHead *pMsgHead = (EasyCamMsgHead* )pParsePoint;
						memcpy( magic, pMsgHead->magic, 8 );
						if( strncmp(magic, "LINKWIL", 8) )
						{
							//LOGE("magic[%c][%c][%c][%c][%c][%c][%c][%c] error, parsed len:%d",
							//		magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6], magic[7], parsedLen);
							parsedLen += 1;
							continue;
						}

						//DEBUG_INFO("Magic:%s, id:%d, dataLen:%d, remainLen:%d", magic, pMsgHead->id, pMsgHead->dataLen, remainLen);
						if( (pMsgHead->dataLen + sizeof(EasyCamMsgHead)) > (unsigned int)remainLen ) // Msg not complete
						{
							//LOGI("Msg not complete ,remain:%d, need:%d", remainLen, (pMsgHead->dataLen+sizeof(EasyCamMsgHead)) );
							break;
						}

						msgDataLen = (pMsgHead->dataLen + sizeof(EasyCamMsgHead));
						parsedLen += msgDataLen;

						// Handle msg
						pThis->handleMsg( pThis->mSockHandle, pParsePoint, msgDataLen);
						if( retryCnt++ > 1000 )
						{
							LOGE("Too many data in message buffer, now drop all data");
							parsedLen = cmdRecvBufSize; // drop all data
							break;
						}
					}

					if( parsedLen >0 )
					{
						int remainDataLen = cmdRecvBufSize - parsedLen;
						char * pRemainData = cmdRecvBuf + parsedLen;
						memmove( cmdRecvBuf, pRemainData, remainDataLen );
						cmdRecvBufSize = remainDataLen;
					}
				}
			}
			else
			{
				// Remote disconnected
				LOGE("PPCS_Read fail channel 0, ret:%d, fd:%d", ret, pThis->mSockHandle);
				if( !pThis->mHasBrokenDetected )
				{
					pThis->mHasBrokenDetected = true;
					if( pThis->mInitInfo.lpConnectionBroken != NULL )
					{
						pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle, pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_CMD);
					}
				}
				break;
			}
		}
		else
		{
			usleep(10000);
		}
	}

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
	DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
	pthread_mutex_unlock( &pThis->mJNIEnvMutex );

	if( cmdRecvBuf )
	{
	    delete []cmdRecvBuf;
	}

	LOGD("Leave cmdRecvThread, fd:%d", pThis->mSockHandle);

	return NULL;
}

void* CEasyCamClient::avUdpRecvThread(void* param)
{
    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"avUdpRecvThread");
#endif
#endif

    CEasyCamClient* pThis = (CEasyCamClient* )param;
    char *cmdRecvBuf = new char[1024*1024];
    char *buf = new char[20480];
    int dataSize = 0;
    int ret = 0;
    
    LOGD("Enter av udp recv thread, fd:%d", pThis->mSockHandle);
    
    //LOGD("Attach av thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("AV thread env attached");
    
    while( pThis->mAVRecvThreadRun )
    {
        //LOGD("mAVRecvThreadRun:%d", pThis->mAVRecvThreadRun);
        if( pThis->mSockHandle >= 0 )
        {
            dataSize = 20480;
            
            ret = LINK_PktRecv(pThis->mSockHandle, 3, buf, &dataSize, 20); // timeout 20ms
            if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
            {
                if( dataSize > 0 )
                {
                    int *pIndex = (int*)buf;
                    LOGI("=== PPCS_PktRecv, dataSize:%d, buf:%d", dataSize, *pIndex);
                }else
                {
                    
                }
                
                usleep(10*1000);        //10ms
            }
            else
            {
                LOGE("PPCS_Read fail channel 3, ret:%d, fd:%d", ret, pThis->mSockHandle);
                
                if( !pThis->mHasBrokenDetected )
                {
                    pThis->mHasBrokenDetected = true;
                    
                    // Remote disconnected
                    if( pThis->mInitInfo.lpConnectionBroken != NULL )
                    {
                        pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle,
                                                            pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_AV);
                    }
                }
                break;
            }
        }
        else
        {
            usleep(10000);
        }
    }
    
    LOGD("Leave av udp recv thread, fd:%d", pThis->mSockHandle);
    
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    
    if( cmdRecvBuf )
    {
        delete []cmdRecvBuf;
    }
    
    if( buf )
    {
        delete []buf;
    }
    
    return NULL;

}

void* CEasyCamClient::avRecvThread(void* param)
{
	CEasyCamClient* pThis = (CEasyCamClient* )param;

    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"avRecvThread");
#endif
#endif

	char *cmdRecvBuf = new char[1024*1024];
	int cmdRecvBufSize = 0;
	char *buf = new char[20480];
	int dataSize = 0;
	int ret = 0;

	LOGD("Enter av recv thread, fd:%d", pThis->mSockHandle);

	//LOGD("Attach av thread env, pid:%#X", pthread_self());
	pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("AV thread env attached");

	while( pThis->mAVRecvThreadRun )
	{
	    //LOGD("mAVRecvThreadRun:%d", pThis->mAVRecvThreadRun);
		if( pThis->mSockHandle >= 0 )
		{
			dataSize = 20480;
			ret = LINK_Read(pThis->mSockHandle, 1, buf, &dataSize, 20); // timeout 20ms
			//LOGE("AV PPCS_Read:%d, session handle:%d, channel:1, dataSize:%d",
			//    ret, pThis->mSockHandle, dataSize);
			if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
			{
				if( dataSize > 0 )
				{
					memcpy( cmdRecvBuf+cmdRecvBufSize, buf, dataSize );
					cmdRecvBufSize += dataSize;

					// parse data
					int parsedLen = 0;
					char* pParsePoint = NULL;
					int remainLen = 0;
					int msgDataLen = 0;
					char magic[16] = {'\0'};
					int retryCnt = 0;
					while( pThis->mAVRecvThreadRun )
					{
						pParsePoint = cmdRecvBuf+parsedLen;
						remainLen = cmdRecvBufSize - parsedLen;
						if( (remainLen<=0) || (unsigned int)remainLen < sizeof(EasyCamMsgHead) )
						{
							// no more data to parse
							//LOGI("No more data to parse ,remain:%d", remainLen);
							break;
						}

						EasyCamMsgHead *pMsgHead = (EasyCamMsgHead* )pParsePoint;
						memcpy( magic, pMsgHead->magic, 8 );
						if( strcmp(magic, "LINKWIL") )
						{
							//LOGE("magic[%c][%c][%c][%c][%c][%c][%c][%c] error, parsed len:%d",
							//		magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6], magic[7], parsedLen);
							parsedLen += 1;
							continue;
						}

						//LOGI("Magic:%s, id:%d, dataLen:%d, remainLen:%d", magic, pMsgHead->msgId, pMsgHead->dataLen, remainLen);
						if( (pMsgHead->dataLen + sizeof(EasyCamMsgHead)) > (unsigned int)remainLen ) // Msg not complete
						{
							//LOGI("Msg not complete ,remain:%d, need:%d", remainLen, (pMsgHead->dataLen+sizeof(EasyCamMsgHead)) );
							break;
						}

						msgDataLen = (pMsgHead->dataLen + sizeof(EasyCamMsgHead));
						parsedLen += msgDataLen;

						// Handle msg
						pThis->handleMsg( pThis->mSockHandle, pParsePoint, msgDataLen);
						if( retryCnt++ > 1000 )
						{
							LOGE("Too many data in message buffer, now drop all data");
							parsedLen = cmdRecvBufSize; // drop all data
							break;
						}
					}

					if( parsedLen >0 )
					{
						int remainDataLen = cmdRecvBufSize - parsedLen;
						char * pRemainData = cmdRecvBuf + parsedLen;
						memmove( cmdRecvBuf, pRemainData, remainDataLen );
						cmdRecvBufSize = remainDataLen;
					}
				}
			}
			else
			{
				LOGE("PPCS_Read fail channel 1, ret:%d, fd:%d", ret, pThis->mSockHandle);

				if( !pThis->mHasBrokenDetected )
				{
					pThis->mHasBrokenDetected = true;

					// Remote disconnected
					if( pThis->mInitInfo.lpConnectionBroken != NULL )
					{
						pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle,
								pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_AV);
					}
				}
				break;
			}
		}
		else
		{
			usleep(10000);
		}
	}

	LOGD("Leave av recv thread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
	DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
	pthread_mutex_unlock( &pThis->mJNIEnvMutex );

	if( cmdRecvBuf )
	{
	    delete []cmdRecvBuf;
	}

	if( buf )
	{
	    delete []buf;
	}

	return NULL;
}


void* CEasyCamClient::pbRecvThread(void* param)
{
    
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"pbRecvThread");
#endif
#endif

    PlaybackThreadContextInfo* pContext = (PlaybackThreadContextInfo* )param;
	CEasyCamClient* pThis = pContext->pThis;

	char *cmdRecvBuf = new char[1024*1024];
	int cmdRecvBufSize = 0;
	char *buf = new char[20480];
	int dataSize = 0;
	int ret = 0;

	LOGD("Enter pb recv thread, fd:%d", pThis->mSockHandle);

	//LOGD("Attach av thread env, pid:%#X", pthread_self());
	pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("AV thread env attached");

	while( pThis->mPBRecvThreadRun )
	{
		if( pThis->mSockHandle >= 0 )
		{
			dataSize = 20480;
			ret = LINK_Read(pThis->mSockHandle, 2, buf, &dataSize, 100); // timeout 100ms
			if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
			{
				if( dataSize > 0 )
				{
					memcpy( cmdRecvBuf+cmdRecvBufSize, buf, dataSize );
					cmdRecvBufSize += dataSize;

					// parse data
					int parsedLen = 0;
					char* pParsePoint = NULL;
					int remainLen = 0;
					int msgDataLen = 0;
					char magic[16] = {'\0'};
					int retryCnt = 0;
					while( pThis->mPBRecvThreadRun )
					{
						pParsePoint = cmdRecvBuf+parsedLen;
						remainLen = cmdRecvBufSize - parsedLen;
						if( (remainLen<=0) || (unsigned int)remainLen < sizeof(EasyCamMsgHead) )
						{
							// no more data to parse
							//LOGI("No more data to parse ,remain:%d", remainLen);
							break;
						}

						EasyCamMsgHead *pMsgHead = (EasyCamMsgHead* )pParsePoint;
						memcpy( magic, pMsgHead->magic, 8 );
						if( strcmp(magic, "LINKWIL") )
						{
//                            LOGE("magic[%c][%c][%c][%c][%c][%c][%c][%c] error, parsed len:%d",
//                                    magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6], magic[7], parsedLen);
							parsedLen += 1;
							continue;
						}

						//LOGI("Magic:%s, id:%d, dataLen:%d, remainLen:%d", magic, pMsgHead->msgId, pMsgHead->dataLen, remainLen);
						if( (pMsgHead->dataLen + sizeof(EasyCamMsgHead)) > (unsigned int)remainLen ) // Msg not complete
						{
							//LOGI("Msg not complete ,remain:%d, need:%d", remainLen, (pMsgHead->dataLen+sizeof(EasyCamMsgHead)) );
							break;
						}

						msgDataLen = (pMsgHead->dataLen + sizeof(EasyCamMsgHead));
						parsedLen += msgDataLen;

						// Handle msg
						pThis->handleMsg( pThis->mSockHandle, pParsePoint, msgDataLen);
						if( retryCnt++ > 1000 )
						{
							LOGE("Too many data in message buffer, now drop all data");
							parsedLen = cmdRecvBufSize; // drop all data
							break;
						}
					}

					if( parsedLen >0 )
					{
						int remainDataLen = cmdRecvBufSize - parsedLen;
						char * pRemainData = cmdRecvBuf + parsedLen;
						memmove( cmdRecvBuf, pRemainData, remainDataLen );
						cmdRecvBufSize = remainDataLen;
					}
				}
			}
			else
			{
				LOGE("PPCS_Read fail channel 2, ret:%d, fd:%d", ret, pThis->mSockHandle);

				if( !pThis->mHasBrokenDetected )
				{
					pThis->mHasBrokenDetected = true;

					// Remote disconnected
					if( pThis->mInitInfo.lpConnectionBroken != NULL )
					{
						pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle,
								pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_PLAYBACK);
					}
				}
				break;
			}
		}
		else
		{
			usleep(10000);
		}
	}

	LOGD("Leave pb recv thread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
	DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
	pthread_mutex_unlock( &pThis->mJNIEnvMutex );

	if( cmdRecvBuf )
	{
	    delete []cmdRecvBuf;
	}

	if( buf )
	{
	    delete []buf;
	}

    if( pContext != NULL )
    {
        delete pContext;
    }

	return NULL;
}

void* CEasyCamClient::fileDownloadRecvThread(void* param)
{
#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"fileDownloadRecvThread");
#endif
#endif


    FileDownloadThreadContextInfo* pContext = (FileDownloadThreadContextInfo* )param;
    CEasyCamClient* pThis = pContext->pThis;

    char *cmdRecvBuf = new char[100*1024];
    int cmdRecvBufSize = 0;
    char *buf = new char[10240];
    int dataSize = 0;
    int ret = 0;

    LOGD("Enter file download recv thread, fd:%d", pThis->mSockHandle);

    //LOGD("Attach av thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("AV thread env attached");

    while( pThis->mFileDownloadThreadRun )
    {
        if( pThis->mSockHandle >= 0 )
        {
            dataSize = 10240;
            ret = LINK_Read(pThis->mSockHandle, 3, buf, &dataSize, 100); // timeout 100ms
            if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
            {
                if( dataSize > 0 )
                {
                    memcpy( cmdRecvBuf+cmdRecvBufSize, buf, dataSize );
                    cmdRecvBufSize += dataSize;

                    // parse data
                    int parsedLen = 0;
                    char* pParsePoint = NULL;
                    int remainLen = 0;
                    int msgDataLen = 0;
                    char magic[16] = {'\0'};
                    int retryCnt = 0;
                    while( pThis->mFileDownloadThreadRun )
                    {
                        pParsePoint = cmdRecvBuf+parsedLen;
                        remainLen = cmdRecvBufSize - parsedLen;
                        if( (remainLen<=0) || (unsigned int)remainLen < sizeof(EasyCamMsgHead) )
                        {
                            // no more data to parse
                            //LOGI("No more data to parse ,remain:%d", remainLen);
                            break;
                        }

                        EasyCamMsgHead *pMsgHead = (EasyCamMsgHead* )pParsePoint;
                        memcpy( magic, pMsgHead->magic, 8 );
                        if( strcmp(magic, "LINKWIL") )
                        {
//                            LOGE("magic[%c][%c][%c][%c][%c][%c][%c][%c] error, parsed len:%d",
//                                    magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6], magic[7], parsedLen);
                            parsedLen += 1;
                            continue;
                        }

                        //LOGI("Magic:%s, id:%d, dataLen:%d, remainLen:%d", magic, pMsgHead->msgId, pMsgHead->dataLen, remainLen);
                        if( (pMsgHead->dataLen + sizeof(EasyCamMsgHead)) > (unsigned int)remainLen ) // Msg not complete
                        {
                            //LOGI("Msg not complete ,remain:%d, need:%d", remainLen, (pMsgHead->dataLen+sizeof(EasyCamMsgHead)) );
                            break;
                        }

                        msgDataLen = (pMsgHead->dataLen + sizeof(EasyCamMsgHead));
                        parsedLen += msgDataLen;

                        // Handle msg
                        pThis->handleMsg( pThis->mSockHandle, pParsePoint, msgDataLen);
                        if( retryCnt++ > 1000 )
                        {
                            LOGE("Too many data in message buffer, now drop all data");
                            parsedLen = cmdRecvBufSize; // drop all data
                            break;
                        }
                    }

                    if( parsedLen >0 )
                    {
                        int remainDataLen = cmdRecvBufSize - parsedLen;
                        char * pRemainData = cmdRecvBuf + parsedLen;
                        memmove( cmdRecvBuf, pRemainData, remainDataLen );
                        cmdRecvBufSize = remainDataLen;
                    }
                }
            }
            else
            {
                LOGE("PPCS_Read fail channel 3, ret:%d, fd:%d", ret, pThis->mSockHandle);

                if( !pThis->mHasBrokenDetected )
                {
                    pThis->mHasBrokenDetected = true;

                    // Remote disconnected
                    if( pThis->mInitInfo.lpConnectionBroken != NULL )
                    {
                        pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle,
                                                            pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_FILEDOWNLOAD);
                    }
                }
                break;
            }
        }
        else
        {
            usleep(10000);
        }
    }

    LOGD("Leave file download recv thread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );

    if( cmdRecvBuf )
    {
        delete []cmdRecvBuf;
    }

    if( buf )
    {
        delete []buf;
    }

    if( pContext != NULL )
    {
        delete pContext;
    }

    return NULL;
}

void* CEasyCamClient::PIRDataRecvThread(void* param)
{
    CEasyCamClient* pThis = (CEasyCamClient* )param;


#ifndef _IOS
#ifndef _WIN32
    prctl(PR_SET_NAME,"PIRDataRecvThread");
#endif
#endif
    
    char *cmdRecvBuf = new char[100*1024];
    int cmdRecvBufSize = 0;
    char *buf = new char[10240];
    int dataSize = 0;
    int ret = 0;

    LOGD("Enter PIR data recv thread, fd:%d", pThis->mSockHandle);

    //LOGD("Attach av thread env, pid:%#X", pthread_self());
    pthread_mutex_lock( &pThis->mJNIEnvMutex );
    ATTACH_JNI_ENV( g_JVM, pthread_self() );
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );
    //LOGD("AV thread env attached");

    while( pThis->mIsPIRDataRecvThreadRun )
    {
        if( pThis->mSockHandle >= 0 )
        {
            dataSize = 10240;
            ret = LINK_Read(pThis->mSockHandle, 4, buf, &dataSize, 20); // timeout 20ms
            if( (ret==ERROR_LINK_SUCCESSFUL) || (ret==ERROR_LINK_TIME_OUT) )
            {
                if( dataSize > 0 )
                {
                    memcpy( cmdRecvBuf+cmdRecvBufSize, buf, dataSize );
                    cmdRecvBufSize += dataSize;

                    // parse data
                    int parsedLen = 0;
                    char* pParsePoint = NULL;
                    int remainLen = 0;
                    int msgDataLen = 0;
                    char magic[16] = {'\0'};
                    int retryCnt = 0;
                    while( pThis->mIsPIRDataRecvThreadRun )
                    {
                        pParsePoint = cmdRecvBuf+parsedLen;
                        remainLen = cmdRecvBufSize - parsedLen;
                        if( (remainLen<=0) || (unsigned int)remainLen < sizeof(EasyCamMsgHead) )
                        {
                            // no more data to parse
                            //LOGI("No more data to parse ,remain:%d", remainLen);
                            break;
                        }

                        EasyCamMsgHead *pMsgHead = (EasyCamMsgHead* )pParsePoint;
                        memcpy( magic, pMsgHead->magic, 8 );
                        if( strcmp(magic, "LINKWIL") )
                        {
                            //LOGE("magic[%c][%c][%c][%c][%c][%c][%c][%c] error, parsed len:%d",
                            //        magic[0], magic[1], magic[2], magic[3], magic[4], magic[5], magic[6], magic[7], parsedLen);
                            parsedLen += 1;
                            continue;
                        }

                        //LOGI("Magic:%s, id:%d, dataLen:%d, remainLen:%d", magic, pMsgHead->msgId, pMsgHead->dataLen, remainLen);
                        if( (pMsgHead->dataLen + sizeof(EasyCamMsgHead)) > (unsigned int)remainLen ) // Msg not complete
                        {
                            //LOGI("Msg not complete ,remain:%d, need:%d", remainLen, (pMsgHead->dataLen+sizeof(EasyCamMsgHead)) );
                            break;
                        }

                        msgDataLen = (pMsgHead->dataLen + sizeof(EasyCamMsgHead));
                        parsedLen += msgDataLen;

                        // Handle msg
                        pThis->handleMsg( pThis->mSockHandle, pParsePoint, msgDataLen);
                        if( retryCnt++ > 1000 )
                        {
                            LOGE("Too many data in message buffer, now drop all data");
                            parsedLen = cmdRecvBufSize; // drop all data
                            break;
                        }
                    }

                    if( parsedLen >0 )
                    {
                        int remainDataLen = cmdRecvBufSize - parsedLen;
                        char * pRemainData = cmdRecvBuf + parsedLen;
                        memmove( cmdRecvBuf, pRemainData, remainDataLen );
                        cmdRecvBufSize = remainDataLen;
                    }
                }
            }
            else
            {
                LOGE("PPCS_Read fail channel 4, ret:%d, fd:%d", ret, pThis->mSockHandle);

                if( !pThis->mHasBrokenDetected )
                {
                    pThis->mHasBrokenDetected = true;

                    // Remote disconnected
                    if( pThis->mInitInfo.lpConnectionBroken != NULL )
                    {
                        pThis->mInitInfo.lpConnectionBroken(pThis->mSessionHandle,
                                                            pThis->mSockHandle, CONNECTION_BROKEN_ISSUE_RECV_FILEDOWNLOAD);
                    }
                }
                break;
            }
        }
        else
        {
            usleep(10000);
        }
    }

    LOGD("Leave file download recv thread, fd:%d", pThis->mSockHandle);

    pthread_mutex_lock( &pThis->mJNIEnvMutex );
#ifdef _ANDROID
    DETACH_JNI_ENV(g_JVM, pthread_self());
#endif
    pthread_mutex_unlock( &pThis->mJNIEnvMutex );

    if( cmdRecvBuf )
    {
        delete []cmdRecvBuf;
    }

    if( buf )
    {
        delete []buf;
    }

    return NULL;
}



