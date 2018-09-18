package com.dps.ppcs_api;

@SuppressWarnings("JniMissingFunction")
public class DPS_API {

    public static final int ERROR_DPS_Successful = 0;    //API is successfully executed
    public static final int ERROR_DPS_NotInitialized = -1;    //DPS_Inistialize is not called yet
    public static final int ERROR_DPS_AlreadyInitialized = -2;    //DPS_Inistialize is called already
    public static final int ERROR_DPS_TimeOut = -3;    //Time out
    public static final int ERROR_DPS_FailedToResolveHostName = -4;    //Can¡¯t resolve Server Name
    public static final int ERROR_DPS_FailedToCreateSocket = -5;    //Socket create failed
    public static final int ERROR_DPS_FailedToBindPort = -6;    //Socket bind failed
    public static final int ERROR_DPS_FailedToConnectServer = -7;    //Connection to DPS Server failed
    public static final int ERROR_DPS_FailedToRecvData = -8;    //Failed to Receive data from Server
    public static final int ERROR_DPS_NotEnoughBufferSize = -9;    //Buffer size is not enough
    public static final int ERROR_DPS_InvalidAES128Key = -10;    //AES128Key string must exactly 16 Byte
    public static final int ERROR_DPS_InvalidToken = -11;    //The Token string is not valid
    public static final int ERROR_DPS_OnRecvNotify = -12;    //You can¡¯t call this function while DPS_RecvNotify() is running
    public static final int ERROR_DPS_OnAcquireToken = -13;    //You can¡¯t call this function while DPS_TokenAcquire() is running
    public static final int ERROR_DPS_NotOnRecvNotify = -14;        //You can¡¯t call this function while DPS_RecvNotify() is not running

    public static native String DPS_GetAPIVersion(int[] Version);

    public static native int DPS_Initialize(String ServerIP, int ServerPort, String AES128Key, int PortNo);

    public static native int DPS_DeInitialize();

    public static native int DPS_TokenAcquire(byte[] TokenBuf, int Size);

    public static native int DPS_RecvNotify(String Token, byte[] NotifyContent, int[] Size, int TimeOut_ms);

    public static native int DPS_GetLastAliveTime(int[] Time_Sec);

    static {
        try {
            System.loadLibrary("DPS_API_PPCS");
        } catch (UnsatisfiedLinkError ule) {
            System.out.println("!!!!! loadLibrary(DPS_API_PPCS), Error:" + ule.getMessage());
        }
    }

}

