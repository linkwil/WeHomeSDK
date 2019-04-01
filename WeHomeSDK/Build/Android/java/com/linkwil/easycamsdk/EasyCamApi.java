package com.linkwil.easycamsdk;

import android.util.Log;
import java.util.concurrent.CopyOnWriteArraySet;

/**
 * Created by xiao on 2017/3/10.
 */

public final class EasyCamApi {
    private final static String TAG = "EC_SDK";

    public static final int LOGIN_RESULT_SUCCESS = 0;
    public static final int LOGIN_RESULT_CONNECT_FAIL = -1;
    public static final int LOGIN_RESULT_AUTH_FAIL = -2;
    public static final int LOGIN_RESULT_EXCEED_MAX_CONNECTION = -3;
    public static final int LOGIN_RESULT_RESULT_FORMAT_ERROR = -4;
    public static final int LOGIN_RESULT_FAIL_UNKOWN = -5;
    public static final int LOGIN_RESULT_FAIL_TIMEOUT = -6;

    public static final int CONNECT_TYPE_LAN = (0x01);
    public static final int CONNECT_TYPE_P2P = (0x01<<1);
    public static final int CONNECT_TYPE_RELAY = (0x01<<2);

    public static final int  CMD_EXEC_RESULT_SUCCESS = 0;
    public static final int  CMD_EXEC_RESULT_FORMAT_ERROR = -1;
    public static final int  CMD_EXEC_RESULT_SEND_FAIL = -2;
    public static final int  CMD_EXEC_RESULT_AUTH_FAIL = -3;
    public static final int  CMD_EXEC_RESULT_FAIL_UNKOWN = -4;

    public static final int  ONLINE_QUERY_RESULT_SUCCESS = 0;   // 查询成功
    public static final int  ONLINE_QUERY_RESULT_FAIL_ALREADY_RUNNING = -1;  // 当前UID正在查询中
    public static final int  ONLINE_QUERY_RESULT_FAIL_SERVER_NO_RESP = -2;  // 服务器无响应
    public static final int  ONLINE_QUERY_RESULT_FAIL_UNKOWN = -3; // 未知错误

    private static EasyCamApi mInstance = null;

    private CopyOnWriteArraySet<Object[]> mLogInResultCallbackSet;
    private EasyCamAVStreamCallback mAVStreamCallback;
    private CopyOnWriteArraySet<Object[]> mCommandResultCallbackSet;
    private EasyCamPlaybackStreamCallback mPBAVStreamCallback;
    private EasyCamFileDownloadDataCallback mFileDownloadDataCallback;
    private PIRDataCallback mPIRDataCallback;
    private OnlineStatusQueryResultCallback mOnlineQueryResultCallback;

    public interface PayloadType{
        int PAYLOAD_TYPE_VIDEO_H264 = 0x00;
        int PAYLOAD_TYPE_VIDEO_H265 = 0x01;
        int PAYLOAD_TYPE_VIDEO_MJPEG = 0x02;
        int PAYLOAD_TYPE_AUDIO_PCM = 0x10;
        int PAYLOAD_TYPE_AUDIO_ADPCM = 0x11;
        int PAYLOAD_TYPE_AUDIO_G711 = 0x12;
        int PAYLOAD_TYPE_AUDIO_AAC = 0x14;
        int PAYLOAD_TYPE_AUDIO_G726 = 0x15;
    }

    public class DevListInfo{
        public int devType;
        public String uid;
        public String devName;
        public String fwVer;
    }

    public interface LoginResultCallback{
        void onLoginResult(int handle, int errorCode, int notificationToken, int isCharging, int batPercent);
    }

    public interface CommandResultCallback{
        void onCommandResult(int handle, int errCode, String data, int seq);
    }

    public interface EasyCamAVStreamCallback {
        int FRAME_TYPE_P = 0;
        int FRAME_TYPE_I = 1;

        void receiveVideoStream(int handle, byte[] data, int len, int payloadType, long timestamp, int frameType, int videoWidth, int videoHeight, int wifiQuality);

        void receiveAudioStream(int handle, byte[] data, int len, int payloadType, long timestamp);
    }

    public interface EasyCamPlaybackStreamCallback {
        int FRAME_TYPE_P = 0;
        int FRAME_TYPE_I = 1;

        void receiveVideoStream(int handle, byte[] data, int len, int payloadType, long timestamp, int frameType, int videoWidth, int videoHeight, int session);

        void receiveAudioStream(int handle, byte[] data, int len, int payloadType, long timestamp, int session);

        void playRecordEnd(int handle, int session);
    }

    public interface EasyCamFileDownloadDataCallback{
        void receiveData(int handle, byte[] data, int len, int sessionNo);
    }

    public interface PIRDataCallback{
        void onPIRData(int handle, int timeMS, int adc);
    }

    public interface OnlineStatusQueryResultCallback{
        void onQueryResult(String uid, int result, int lastLoginSec);
    }

    public static EasyCamApi getInstance(){
        if( mInstance == null ){
            mInstance = new EasyCamApi();
        }
        return mInstance;
    }

    private EasyCamApi(){
        mLogInResultCallbackSet = new CopyOnWriteArraySet<>();
        mCommandResultCallbackSet = new CopyOnWriteArraySet<>();
    }

    public boolean addLogInResultCallback(LoginResultCallback callback, int seq) throws NullPointerException{
        Object[] objs = new Object[2];
        objs[0] = callback;
        objs[1] = seq;
        return mLogInResultCallbackSet.add(objs);
    }

    public boolean removeLogInResultCallback(LoginResultCallback callback){
        for (Object[] objs : mLogInResultCallbackSet) {
            if (objs[0] == callback) {
                return mLogInResultCallbackSet.remove(objs);
            }
        }
        return false;
    }

    public boolean addCommandResultCallback(CommandResultCallback callback, int seq){
        Object[] objs = new Object[2];
        objs[0] = callback;
        objs[1] = seq;
        return mCommandResultCallbackSet.add(objs);
    }

    public boolean removeCommandResultCallback(CommandResultCallback callback){
        for (Object[] objs : mCommandResultCallbackSet) {
            if (objs[0] == callback) {
                return mCommandResultCallbackSet.remove(objs);
            }
        }

        return false;
    }

    public void setAVStreamCallback(EasyCamAVStreamCallback callback){
        mAVStreamCallback = callback;
    }

    public EasyCamAVStreamCallback getAVStreamCallback(){
        return mAVStreamCallback;
    }


    public void setPBAVStreamCallback(EasyCamPlaybackStreamCallback callback){
        mPBAVStreamCallback = callback;
    }

    public EasyCamPlaybackStreamCallback getPBAVStreamCallback(){
        return mPBAVStreamCallback;
    }

    public void setFileDownloadDataCallback(EasyCamFileDownloadDataCallback callback){
        mFileDownloadDataCallback = callback;
    }

    public EasyCamFileDownloadDataCallback getFileDownloadDataCallback(){
        return mFileDownloadDataCallback;
    }

    public void setPIRDataCallback(PIRDataCallback callback){
        mPIRDataCallback = callback;
    }

    public PIRDataCallback getPIRDataCallback(){
        return mPIRDataCallback;
    }

    public void setOnlineQueryResultCallback(OnlineStatusQueryResultCallback callback){
        mOnlineQueryResultCallback = callback;
    }

    //=========================================================
    //   Native API
    //=========================================================
    public native String getSdkVersion();

    public native int init();

    public native int onlineStatusQuery(String uid);

    // Return value: session handle
    public native int logIn(String uid, String password, String bcastAddr, int seq, int needVideo, int needAudio, int connecType, int timeOutSec);

    public native int logOut(int handle);

    public native int startSearchDev(int timeoutMS, String bCastAddr);

    public native int stopSearchDev();

    public native DevListInfo[] getDevList();

    public native int sendCommand(int handle, String command, int len, int seq);

    public native int sendTalkData(int handle, byte[] data, int len, int payloadType, int seq);

    public native int networkDetect(String uid);

    public native int [] YUV420P2ARGB8888(byte []buf, int width, int height);

    public native int subscribeMessage(String uid, String appName, String agName, String phoneToken, String devName, int eventCh);

    public native int unSubscribe(String uid, String appName, String agName, String phoneToken, int eventCh);

    public native int resetBadge(String uid, String appName, String agName, String phoneToken, int eventCh);

    //==========================Callback methods==================
    // The following Callbacks is called by JNI
    // Please DON'T change the callback name
    //============================================================
    static void onLoginResult(int handle, int errorCode, int seq, int notificationToken, int isCharging, int batPercent)
    {
        Log.d(TAG, "onLoginResult, handle:"+handle+",errorCode:"+errorCode+
                ",seq:"+seq+",notificationToken:"+notificationToken+",batPercent:"+batPercent);

        CopyOnWriteArraySet<Object[]> callbackSets = EasyCamApi.getInstance().
                mLogInResultCallbackSet;
        for (Object[] objs : callbackSets) {
            LoginResultCallback callback = (LoginResultCallback ) objs[0];
            int sequence = (int) objs[1];
            if( sequence == seq ){
                callback.onLoginResult(handle, errorCode, notificationToken, isCharging, batPercent);
            }
        }
    }
    static void onCmdResult(int handle, int errCode, byte[] data, int seq)
    {
        Log.d(TAG, "onCmdResult, handle:"+handle+",errCode:"+errCode+",seq:"+seq);
        CopyOnWriteArraySet<Object[]> callbackSets = EasyCamApi.getInstance().
                mCommandResultCallbackSet;
        for (Object[] objs : callbackSets) {
            CommandResultCallback callback = (CommandResultCallback ) objs[0];
            int sequence = (int) objs[1];
            if( sequence == seq ){
                String commandResult = new String(data);
                callback.onCommandResult(handle, errCode, commandResult, seq);
            }
        }
    }
    static void OnAudio_RecvData(int handle, byte []data, int len, int payloadType,
                                 long timestamp, int seq)
    {
        //Log.d("LinkBell", "OnAudio_RecvData, handle:"+handle+",seq:"+seq+",len:"+len);
        EasyCamAVStreamCallback callback = EasyCamApi.getInstance().getAVStreamCallback();
        if( callback != null ){
            callback.receiveAudioStream(handle, data, len, payloadType, timestamp);
        }
    }

    static void OnVideo_RecvData(int handle, byte[] data, int len, int payloadType,
                                 long timestamp, int seq, int frameType, int videoWidth, int videoHeight, int wifiQuality)
    {
        //Log.d("LinkBell", "OnVideo_RecvData, handle:"+handle+",seq:"+seq+",len:"+len+",frameType:"+frameType);
        EasyCamAVStreamCallback callback = EasyCamApi.getInstance().getAVStreamCallback();
        if( callback != null ){
            callback.receiveVideoStream(handle, data, len, payloadType, timestamp, frameType, videoWidth, videoHeight, wifiQuality);
        }
    }

    static void OnPBAudio_RecvData(int handle, byte []data, int len, int payloadType,
                                   long timestamp, int seq, int sessionNo)
    {
        //Log.d("LinkBell", "OnAudio_PBRecvData, len:"+len+",timestamp:"+timestamp);
        EasyCamPlaybackStreamCallback callback = EasyCamApi.getInstance().getPBAVStreamCallback();
        if( callback != null ){
            callback.receiveAudioStream(handle, data, len, payloadType, timestamp, sessionNo);
        }
    }

    static void OnPBVideo_RecvData(int handle, byte[] data, int len, int payloadType,
                                 long timestamp, int seq, int frameType, int videoWidth, int videoHeight, int sessionNo)
    {
        //Log.d("LinkBell", "OnPBVideo_RecvData, len:"+len);
        EasyCamPlaybackStreamCallback callback = EasyCamApi.getInstance().getPBAVStreamCallback();
        if( callback != null ){
            callback.receiveVideoStream(handle, data, len, payloadType, timestamp, frameType, videoWidth, videoHeight, sessionNo);
        }
    }

    static void OnPBEnd(int handle, int sessionNo)
    {
        Log.d(TAG, "OnPBEnd, handle:"+handle+",session:"+sessionNo);
        EasyCamPlaybackStreamCallback callback = EasyCamApi.getInstance().getPBAVStreamCallback();
        if( callback != null ){
            callback.playRecordEnd(handle, sessionNo);
        }
    }

    static void OnFileDownload_RecvData(int handle, byte[] data, int len, int sessionNo)
    {
        Log.d(TAG, "OnFileDownload_RecvData, len:"+len+", sessionNo:"+sessionNo);
        if( EasyCamApi.getInstance().mFileDownloadDataCallback != null ){
            EasyCamApi.getInstance().mFileDownloadDataCallback.receiveData(handle, data, len ,sessionNo);
        }
    }

    static void OnPIRData(int handle, int timeMS, int adc)
    {
        //Log.d("LinkBell", "adc:"+adc);
        if( EasyCamApi.getInstance().mPIRDataCallback != null ){
            EasyCamApi.getInstance().mPIRDataCallback.onPIRData(handle, timeMS, adc);
        }
    }

    static void OnlineStatusQueryResult(String uid, int queryResult, int lastLoginSec)
    {
        if( EasyCamApi.getInstance().mOnlineQueryResultCallback != null ){
            EasyCamApi.getInstance().mOnlineQueryResultCallback.onQueryResult(uid,
                    queryResult, lastLoginSec);
        }
    }

    static {
        try {
            System.loadLibrary("EasyCamSdk");
        } catch (UnsatisfiedLinkError ule) {
            Log.e(TAG, "Load libEasyCamSdk.so fail:"+ule.getMessage());
        }
    }
}
