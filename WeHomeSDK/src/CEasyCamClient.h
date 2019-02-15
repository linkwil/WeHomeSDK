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

#ifndef CEASYCAMCLIENT_H_
#define CEASYCAMCLIENT_H_

#include <pthread.h>
#include <list>

#include "audio_process/noise_suppression.h"
#include "audio_process/gain_control.h"

extern "C"
{
#include "adpcm.h"
}

enum CONNECTION_BROKEN_ISSUE
{
	CONNECTION_BROKEN_ISSUE_SEND_CMD,
	CONNECTION_BROKEN_ISSUE_RECV_CMD,
	CONNECTION_BROKEN_ISSUE_RECV_AV,
	CONNECTION_BROKEN_ISSUE_RECV_PLAYBACK,
	CONNECTION_BROKEN_ISSUE_RECV_FILEDOWNLOAD,
};

typedef struct tagCLIENT_INIT_INFO
{
	void (*lpConnectionBroken)(int sessionHandle, int sockHandle, int issue);
	void (*lpLoginResult)(int handle, int errorCode, int seq, unsigned int notificationToken, unsigned int isCharging, unsigned int batPercent, unsigned int supportEncryption);
	void (*lpCmdResult)(int handle, char* data, int seq);
	void (*lpAudio_RecvData)(int handle, char *data, int len, int payloadType, long long timestamp, int seq);
	void (*lpVideo_RecvData)(int handle, char *data, int len, int payloadType, long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, unsigned int wifiQuality);
    void (*lpPBAudio_RecvData)(int handle, char *data, int len, int payloadType, long long timestamp, int seq, int pbSessionNo);
    void (*lpPBVideo_RecvData)(int hanlde, char *data, int len, int payloadType, long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, int pbSessionNo);
    void (*lpPBEnd)(int hanlde, int pbSessionNo);
	void (*lpFileDownload_RecvData)(int handle, char* data, int len, int sessionNo);
    void (*lpPIRData_RecvData)(int handle, long long timeMS, short adc);
}CLIENT_INIT_INFO;


class CEasyCamClient {
protected:
	typedef struct tagPlaybackThreadContextInfo{
		CEasyCamClient* pThis;
		int cmdSeq;
	}PlaybackThreadContextInfo;

	typedef struct tagFileDownloadThreadContextInfo{
		CEasyCamClient* pThis;
		int cmdSeq;
	}FileDownloadThreadContextInfo;

public:
	CEasyCamClient(CLIENT_INIT_INFO* init, int sessionHandle, const char* devUid);
	virtual ~CEasyCamClient();

public:
	int logIn(const char* uid, const char* usrName, const char* password, const char* broadcastAddr, int seq, int needVideo, int needAudio, int connectType, int timeout);
	int logOut(void);

	void notifyStartPlayRecord(void){mNeedWaitPlaybackStartFrame=true;};

	int sendCommand(char* command, int seq);
	int sendTalkData(char* data, int dataLen, int payloadType, int seq);

    unsigned int getToken();
    
    int sendLoginCmd(int sessionHandle);

public:
	void joinThreads(void);
	void closeConnection(void);

public:
    int mIsRsaLogin;
    
protected:
	int addCmdToQueue(void* cmd);
	void* getCmdFromQueue(void);
	void clearCmdQueue(void);

	int addTalkDataToQueue(void* data);
	void* getTalkDataFromQueue(void);
	void clearTalkDataQueue(void);


	void handleCmdLoginResp(char* data, int seq);
	void handleCmdResp(char* data, int seq);

	int handleMsg(int fd, char* data, int len);

protected:
    static void* lanConnectThread(void* );
    static void* p2pConnectThread(void*);
    static void* relayConnectThread(void*);
    
    static void* broadcastThread(void*);
    static void* connectTimeThread(void*);
    
	static void* cmdSendThread(void* );
	static void* audioSendThread(void* );
	static void* cmdRecvThread(void* );
	static void* avRecvThread(void* );
    static void* avUdpRecvThread(void* );
	static void* pbRecvThread(void* );
	static void* fileDownloadRecvThread(void* param);
    static void* PIRDataRecvThread(void* param);

private:
    
    bool mIsConnecting;
    bool mLanIsConnecting;
    bool mP2pIsConnecting;
    bool mRelayIsConnecting;
    bool mConnectTimeRuning;
    
    pthread_t mLanConnectThread;
    pthread_t mP2pConnectThread;
    pthread_t mRelayConnectThread;
    
    int mLanSockHandle;
    int mP2pSockHandle;
    int mRelaySockHandle;
    
    bool mLanIsConnectSuccess;
    bool mP2pIsConnectSuccess;
    bool mRelayIsConnectSuccess;

    pthread_t mBroadcastThread;
    pthread_t mConnectTimeThread; 
    
    bool mBroadcastThreadRun;
    
    int mConnectTimeoutValue;           //second
    
	bool mCmdSendThreadRun;
	pthread_t mCmdSendThread;
	bool mHasCmdSendThreadStarted;

	bool mTalkDataSendThreadRun;
	pthread_t mTalkDataSendThread;
	bool mHasTalkDataSendThreadStarted;

	bool mCmdRecvThreadRun;
	bool mHasCmdRecvThreadStarted;
	pthread_t mCmdRecvThread;

	bool mAVRecvThreadRun;
	bool mHasAVRecvThreadStarted;
	pthread_t mAVRecvThread;

	bool mPBRecvThreadRun;
	bool mHasPBRecvThreadStarted;
	pthread_t mPBRecvThread;

	bool mFileDownloadThreadRun;
	bool mHasFileDownloadThreadStarted;
	pthread_t mFileDownloadThread;

	bool mIsPIRDataRecvThreadRun;
	bool msHasPIRDataRecvThreadStarted;
	pthread_t mPIRDataRecvThread;

	std::list<void* > mCmdQueue;
	pthread_mutex_t mCmdQueueMutex;
	bool mCmdQueueEnable;

	std::list<void* >mTalkDataQueue;
	pthread_mutex_t mTalkDataQueueMutex;
	bool mTalkDataQueueEnable;

	pthread_mutex_t mCmdMsgMutex;

	pthread_mutex_t mJNIEnvMutex;

	char mUid[64];
	char mUsrAccount[64];
	char mUsrPassword[64];
    char mBroadcastAddr[64];
    char mUsrAccountRsaEncoded[256];
    char mUsrPasswordRsaEncoded[256];
    char mAesKey[32];
    char mAesKeyRsaEncoded[256];
	int mLoginSeq;
	int mLoginNeedVideo;
	int mLoginNeedAudio;
    int mConnectType;
    
	bool mNeedBreakConnect;
	int mSockHandle;
	int mSessionHandle;
	bool mHasBrokenDetected;

	bool mNeedWaitPlaybackStartFrame;
    
    unsigned int mToken;

	CLIENT_INIT_INFO mInitInfo;

	adpcm_state mAdpcmDecState;
	char mPCMBuf[1024];

	adpcm_state mAdpcmEncState;
	char mTalkAdpcmBuf[1024];

	NsHandle* mNsHandle;
	char mNsOutBuf[10*1024];
	char mNsBuf[10*1024];
	int mNsBufSize;

	void *mAgcHandle;
	int mMicLevelIn;
    int mMicLevelOut;
    char mTalkAgcOutBuf[10*1024];
};

#endif /* CEASYCAMCLIENT_H_ */
