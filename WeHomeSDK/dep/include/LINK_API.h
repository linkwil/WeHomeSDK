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

#ifndef LINK_API_h
#define LINK_API_h


#ifdef WIN32DLL
#ifdef LINK_API_EXPORTS
#define LINK_API_API __declspec(dllexport)
#else
#define LINK_API_API __declspec(dllimport)
#endif
#endif /* WIN32DLL */

#ifdef LINUX
#include <netinet/in.h>
#define LINK_API_API
#endif /* LINUX */

#ifdef _ARC_COMPILER
#include "net_api.h"
#define LINK_API_API
#endif /* _ARC_COMPILER */

#include "LINK_Type.h"
#include "LINK_Error.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    
    
typedef enum TAG_LINK_SUB_RETURN
{
    LINK_SUB_RETURN_FALES = -1,
    LINK_SUB_RETURN_OK = 0,
}LINK_SUB_RETURN;
    
//query dev and wakeup server connect status
#define ERROR_NotLogin                -1
#define ERROR_InvalidParameter        -2
#define ERROR_SocketCreateFailed      -3
#define ERROR_SendToFailed            -4
#define ERROR_RecvFromFailed          -5
#define ERROR_UnKnown                 -99
#define SERVER_NUM                    3
    
#define ONLINE_QUERY_RESULT_SUCCESS                 (0)    /* */
#define ONLINE_QUERY_RESULT_FAIL_ALREADY_RUNNING    (-1)   /* */
#define ONLINE_QUERY_RESULT_FAIL_SERVER_NO_RESP     (-2)   /* */
#define ONLINE_QUERY_RESULT_FAIL_UNKOWN             (-100) /* */
typedef void(*OnlineQueryResultCallback)(int queryResult, const char* uid, int lastLoginSec);
    
    typedef struct
    {
        CHAR bFlagInternet;        // Internet Reachable? 1: YES, 0: NO
        CHAR bFlagHostResolved;    // P2P Server IP resolved? 1: YES, 0: NO
        CHAR bFlagServerHello;    // P2P Server Hello? 1: YES, 0: NO
        CHAR NAT_Type;            // NAT type, 0: Unknow, 1: IP-Restricted Cone type,   2: Port-Restricted Cone type, 3: Symmetric
        CHAR MyLanIP[16];        // My LAN IP. If (bFlagInternet==0) || (bFlagHostResolved==0) || (bFlagServerHello==0), MyLanIP will be "0.0.0.0"
        CHAR MyWanIP[16];        // My Wan IP. If (bFlagInternet==0) || (bFlagHostResolved==0) || (bFlagServerHello==0), MyWanIP will be "0.0.0.0"
    } st_LINK_NetInfo;
    
    typedef struct
    {
        INT32  Skt;                    // Sockfd
        struct sockaddr_in RemoteAddr;    // Remote IP:Port
        struct sockaddr_in MyLocalAddr;    // My Local IP:Port
        struct sockaddr_in MyWanAddr;    // My Wan IP:Port
        UINT32 ConnectTime;                // Connection build in ? Sec Before
        CHAR DID[24];                    // Device ID
        CHAR bCorD;                        // I am Client or Device, 0: Client, 1: Device
        CHAR bMode;                        // Connection Mode: 0: P2P, 1:Relay Mode
        CHAR Reserved[2];
    } st_LINK_Session;
    
    typedef struct
    {
        CHAR WanIP[16];          // The Wan IP, °∞0.0.0.0°± means Wan IP is not successfully detected.
        CHAR WanIPv6[40];        // The Wan IPv6 IP, available only when IPv6 IP is activated and working.
        CHAR LanIP[16];         // The Local IP , °∞0.0.0.0°± means Lan IP is not successfully detected.
        CHAR LanIPv6[40];        // The Local IPv6 IP , available only when IPv6 IP is activated and working.
        UINT16 LanPort;            // Local UDP Port, 0 means not successfully detected.
        UINT16 WanPort;            // Wan UDP Port , 0 means not successfully detected.
        CHAR bServerHelloAck;    // 0: No server response, 1: server responsed.
        CHAR bReserved[3];        // Reserved ...Should always be "0"
    } st_NDT_LINK_NetInfo;

    LINK_API_API UINT32 LINK_GetAPIVersion(void);
    LINK_API_API INT32 LINK_QueryDID(const CHAR* DeviceName, CHAR* DID, INT32 DIDBufSize);
    LINK_API_API INT32 LINK_Initialize(void);
    LINK_API_API INT32 LINK_DeInitialize(void);
    LINK_API_API INT32 LINK_NetworkDetect(st_LINK_NetInfo *NetInfo, UINT16 UDP_Port);
    LINK_API_API INT32 LINK_NetworkDetectByServer(st_LINK_NetInfo *NetInfo, UINT16 UDP_Port, CHAR *devUid);
    LINK_API_API INT32 LINK_Share_Bandwidth(CHAR bOnOff);
    LINK_API_API INT32 LINK_Listen(const CHAR *MyID, const UINT32 TimeOut_Sec, UINT16 UDP_Port, CHAR bEnableInternet, const CHAR* APILicense);
    LINK_API_API INT32 LINK_Listen_Break(void);
    LINK_API_API INT32 LINK_LoginStatus_Check(CHAR* bLoginStatus);
    LINK_API_API INT32 LINK_Connect(const CHAR *TargetID, CHAR bEnableLanSearch, UINT16 UDP_Port);
    LINK_API_API INT32 LINK_ConnectByServer(const CHAR *TargetID, CHAR bEnableLanSearch, UINT16 UDP_Port);
    LINK_API_API INT32 LINK_Connect_Break(void);
    LINK_API_API INT32 LINK_Check(INT32 SessionHandle, st_LINK_Session *SInfo);
    LINK_API_API INT32 LINK_Close(INT32 SessionHandle);
    LINK_API_API INT32 LINK_ForceClose(INT32 SessionHandle);
    LINK_API_API INT32 LINK_Write(INT32 SessionHandle, UCHAR Channel, CHAR *DataBuf, INT32 DataSizeToWrite);
    LINK_API_API INT32 LINK_Read(INT32 SessionHandle, UCHAR Channel, CHAR *DataBuf, INT32 *DataSize, UINT32 TimeOut_ms);
    LINK_API_API INT32 LINK_Check_Buffer(INT32 SessionHandle, UCHAR Channel, UINT32 *WriteSize, UINT32 *ReadSize);
    //// Ther following functions are available after ver. 2.0.0
    LINK_API_API INT32 LINK_PktSend(INT32 SessionHandle, UCHAR Channel, CHAR *PktBuf, INT32 PktSize); //// Available after ver. 2.0.0
    LINK_API_API INT32 LINK_PktRecv(INT32 SessionHandle, UCHAR Channel, CHAR *PktBuf, INT32 *PktSize, UINT32 TimeOut_ms); //// Available after ver. 2.0.0
    
    LINK_API_API INT32 LINK_WakeUp_Query(
                                         const char* devUid,
                                         const int tryCount,
                                         const unsigned int timeout_ms,
                                         int *LastLogin1,
                                         int *LastLogin2,
                                         int *LastLogin3);
    
    LINK_API_API INT32 LINK_Subscribe(const char* uid, const char* appName,   const char* agName, const char* phoneToken, unsigned int eventCh);
    LINK_API_API INT32 LINK_UnSubscribe(const char* uid, const char* appName,   const char* agName, const char* phoneToken, unsigned int eventCh);
    LINK_API_API INT32 LINK_ResetBadge(const char* uid, const char* appName,   const char* agName, const char* phoneToken, unsigned int eventCh);
 
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* LINK_API_h */
