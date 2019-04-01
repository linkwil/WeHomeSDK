package com.linkwil.ecsdkdemo;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ReplacementTransformationMethod;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Toast;

import com.linkwil.easycamsdk.EasyCamApi;
import com.linkwil.linkbell.sdk.decoder.H264Decoder;
import com.linkwil.service.DemoService;
import com.linkwil.util.ECAudioFrameQueue;
import com.linkwil.util.ECVideoFrameQueue;
import com.linkwil.util.StreamView;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.List;

import static android.Manifest.permission.RECORD_AUDIO;
import static android.os.Build.VERSION_CODES.M;

public class MainActivity extends AppCompatActivity {

    private final int LIVE_VIDEO_FRAME_BUF_SIZE = 60;
    private final int LIVE_AUDIO_FRAME_BUF_SIZE = 255;

    private final int REQUEST_CODE_AUDIO_PERMISSION = 0x100;

    private Button mBtnWiFiConfig;
    private EditText mEditUid;
    private EditText mEditPassword;
    private Button mBtnLogIn;
    private Button mBtnLogout;
    private Button mBtnCmdTest;
    private Button mBtnDpsSubsribe;
    private Button mBtnTalk;
    private FrameLayout mStreamViewerContainer;
    private StreamView mStreamViewer;

    private int mVideoWidth, mVideoHeight;

    private H264Decoder mH264Decoder;
    private final int MAX_VIDEO_FRAME_SIZE = 512*1024;
    private ByteBuffer mH264ByteBuffer;
    private ByteBuffer mRGBByteBuffer;

    private int mLoginSeq = 0;
    private int mSessionHandle = -1;
    private int mCmdSeqNo = 0;

    private final int MSG_ID_LOGIN_FAIL = 0;
    private final int MSG_ID_LOGIN_SUCCESS = 1;
    private final int MSG_ID_SUBSCRIBE_MSG_SUCCESS = 2;
    private final int MSG_ID_SUBSCRIBE_MSG_FAIL = 3;
    private final int MSG_ID_START_TALK_SUCCESS = 4;
    private final int MSG_ID_START_TALK_OCCUPIED = 5;
    private final int MSG_ID_START_TALK_FAILED = 6;
    private final int MSG_ID_CLOSE_TALK_SUCCESS = 7;

    private EasyCamApi.LoginResultCallback mLoginResultCallback;

    private MyHandler mHandler = new MyHandler(this);

    private ProgressDialog mConnectingDlg;

    private IECVideoFrameQueue mVideoFrameQueue = new IECVideoFrameQueue(LIVE_VIDEO_FRAME_BUF_SIZE);
    private VideoQueueProcess mVideoQueueProcess;
    private IECAudioFrameQueue mAudioFrameQueue = new IECAudioFrameQueue(LIVE_AUDIO_FRAME_BUF_SIZE);
    private AudioQueueProcess mAudioQueueProcess;

    private int mNotificationEventChannel = -1;

    private int bufSize = AudioTrack.getMinBufferSize(16000, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
    private AudioTrack mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 16000, AudioFormat.CHANNEL_OUT_MONO,
            AudioFormat.ENCODING_PCM_16BIT, bufSize*2, AudioTrack.MODE_STREAM);

    private AudioRecord mRecorder;
    private  int mAudioBufSize;
    private Thread mAudioRecordThread;
    private boolean mIsAudioThreadRun;

    private boolean isToCmdTestActivity = false;

    private static class MyHandler extends Handler{
        private WeakReference<MainActivity> reference;

        private MyHandler(MainActivity activity) {
            reference = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            MainActivity activity = reference.get();
            if( activity != null )
                activity.handleMsg(msg);
        }
    }

    public void handleMsg(Message msg){
        if( !isFinishing() ){
            if( msg.what == MSG_ID_LOGIN_FAIL ){
                mConnectingDlg.dismiss();

                mBtnLogout.setEnabled(false);
                mBtnCmdTest.setEnabled(false);
                mBtnDpsSubsribe.setEnabled(false);
                mBtnTalk.setEnabled(false);

                EasyCamApi.getInstance().logOut(mSessionHandle);

                String errMsg = null;
                if( msg.arg1 == EasyCamApi.LOGIN_RESULT_CONNECT_FAIL ){
                    errMsg = "Connect fail";
                }else if( msg.arg1 == EasyCamApi.LOGIN_RESULT_AUTH_FAIL ){
                    errMsg = "Auth fail";
                }else if( msg.arg1 == EasyCamApi.LOGIN_RESULT_EXCEED_MAX_CONNECTION ){
                    errMsg = "Exceed max connection";
                }else if( msg.arg1 == EasyCamApi.LOGIN_RESULT_RESULT_FORMAT_ERROR ){
                    errMsg = "Result format error";
                }else if( msg.arg1 == EasyCamApi.LOGIN_RESULT_FAIL_UNKOWN ){
                    errMsg = "Fail unkonwn";
                }else if( msg.arg1 == EasyCamApi.LOGIN_RESULT_FAIL_TIMEOUT ){
                    errMsg = "Connection timeout";
                }
                if( errMsg != null ) {
                    Toast.makeText(this, errMsg, Toast.LENGTH_SHORT).show();
                }
            }else if( msg.what == MSG_ID_LOGIN_SUCCESS ){
                mConnectingDlg.dismiss();

                mBtnLogIn.setEnabled(false);
                mBtnLogout.setEnabled(true);
                mBtnCmdTest.setEnabled(true);
                mBtnDpsSubsribe.setEnabled(true);
                mBtnTalk.setEnabled(true);
            }else if( msg.what == MSG_ID_SUBSCRIBE_MSG_SUCCESS ){
                mConnectingDlg.dismiss();
                Toast.makeText(this, "Notification subscribed successfully", Toast.LENGTH_SHORT).show();
            }else if( msg.what == MSG_ID_SUBSCRIBE_MSG_FAIL ){
                mConnectingDlg.dismiss();
                Toast.makeText(this, "Notification subscribed failed", Toast.LENGTH_SHORT).show();
            }else if( msg.what == MSG_ID_START_TALK_SUCCESS ){
                mConnectingDlg.dismiss();
                // Start audio record thread
                mIsAudioThreadRun = true;
                mAudioRecordThread = new Thread(RecordAudioRunnable);
                mAudioRecordThread.start();
                Toast.makeText(this, "Start talk success", Toast.LENGTH_SHORT).show();
            }else if( msg.what == MSG_ID_START_TALK_OCCUPIED ){
                mConnectingDlg.dismiss();
                mBtnTalk.setSelected(false);
                Toast.makeText(this, "Occupied by another user", Toast.LENGTH_SHORT).show();
            }else if( msg.what == MSG_ID_START_TALK_FAILED ){
                mConnectingDlg.dismiss();
                mBtnTalk.setSelected(false);
                Toast.makeText(this, "Start/stop talk failed", Toast.LENGTH_SHORT).show();
            }else if( msg.what == MSG_ID_CLOSE_TALK_SUCCESS ){
                mConnectingDlg.dismiss();
                Toast.makeText(this, "Stop talk success", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mBtnWiFiConfig = findViewById(R.id.btn_wifi_config);
        mEditUid= findViewById(R.id.et_device_uid);
        mEditPassword = findViewById(R.id.et_device_password);
        mBtnLogIn = findViewById(R.id.btn_login);
        mBtnLogout = findViewById(R.id.btn_logout);
        mBtnCmdTest = findViewById(R.id.btn_cmd_test);
        mBtnDpsSubsribe = findViewById(R.id.btn_dps_subscribe);
        mBtnTalk = findViewById(R.id.btn_talk);
        mStreamViewerContainer = findViewById(R.id.layout_player_container);

        mConnectingDlg = new ProgressDialog(this);

        mBtnWiFiConfig.setOnClickListener(mOnClickListener);
        mBtnLogIn.setOnClickListener(mOnClickListener);
        mBtnLogout.setOnClickListener(mOnClickListener);
        mBtnCmdTest.setOnClickListener(mOnClickListener);
        mBtnDpsSubsribe.setOnClickListener(mOnClickListener);
        mBtnTalk.setOnClickListener(mOnClickListener);

        mBtnLogout.setEnabled(false);
        mBtnCmdTest.setEnabled(false);
        mBtnDpsSubsribe.setEnabled(false);
        mBtnTalk.setEnabled(false);

        mH264Decoder = new H264Decoder(H264Decoder.COLOR_FORMAT_BGR32);
        mH264ByteBuffer = ByteBuffer.allocateDirect(MAX_VIDEO_FRAME_SIZE);
        mAudioTrack.play();

        mStreamViewer = new StreamView(this, Bitmap.Config.ARGB_8888);
        mStreamViewerContainer.addView(mStreamViewer, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mStreamViewer.setForceSoftwareDecode(true);

        SharedPreferences sp = getSharedPreferences("DEMO", MODE_PRIVATE);
        String uid = sp.getString("UID", "");
        if( uid.length() > 0 ){
            mEditUid.setText(uid);
        }

        mEditUid.setTransformationMethod(new ReplacementTransformationMethod() {
            @Override
            protected char[] getOriginal() {
                return new char[]{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
            }

            @Override
            protected char[] getReplacement() {
                return new char[]{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
            }
        });

        EasyCamApi.getInstance().setAVStreamCallback(new EasyCamApi.EasyCamAVStreamCallback() {
            @Override
            public void receiveVideoStream(int handle, byte[] data, int len, int payloadType, long timestamp, int frameType, int videoWidth, int videoHeight, int wifiQuality) {
                //Log.d("DEMO", "Receive video, len:"+len);
                try {
                    mVideoFrameQueue.add(data, len, payloadType, timestamp, frameType,
                            videoWidth, videoHeight, wifiQuality, 0);
                }catch (IllegalStateException e){
                    Log.d("DEMO", "video queue is full");
                }
            }

            @Override
            public void receiveAudioStream(int handle, byte[] data, int len, int payloadType, long timestamp) {
                //Log.d("DEMO", "Receive audio, len:"+len);
                try {
                    mAudioFrameQueue.add(data, len, payloadType, timestamp, 0);
                }catch (IllegalStateException e){
                    Log.d("DEMO", "audio queue is full");
                }
            }
        });

        // start decode process
        mVideoQueueProcess = new VideoQueueProcess("LIVE_VIDEO");
        mVideoQueueProcess.start();
        mAudioQueueProcess = new AudioQueueProcess("LIVE_AUDIO");
        mAudioQueueProcess.start();


        /// Start service
        String serviceName = "com.linkwil.service.LinkBellService";
        //Log.d("LinkBell", "ServiceName:"+serviceName);
        if( !isServiceWork(this, serviceName) ) {
            Log.d("LinkBell", "Start service");
            Intent service = new Intent(this, DemoService.class);
            startService(service);
        }

        EasyCamApi.getInstance().getDevList();
    }

    private boolean isServiceWork(Context context, String serviceName) {
        boolean isWork = false;
        ActivityManager myAM = (ActivityManager) context
                .getSystemService(Context.ACTIVITY_SERVICE);
        if( myAM != null ) {
            List<ActivityManager.RunningServiceInfo> myList = myAM.getRunningServices(256);
            if (myList.size() <= 0) {
                return false;
            }
            for (int i = 0; i < myList.size(); i++) {
                String mName = myList.get(i).service.getClassName();
                if (mName.equals(serviceName)) {
                    isWork = true;
                    break;
                }
            }
        }
        return isWork;
    }


    View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if(v==mBtnWiFiConfig){
                String uid = mEditUid.getText().toString().trim().toUpperCase();
                if( uid.length() < 17 ){
                    Toast.makeText(MainActivity.this, "UID format error", Toast.LENGTH_SHORT).show();
                }else {
                    Intent wifiConfigIntent = new Intent(MainActivity.this, WifiConfigActivity.class);
                    wifiConfigIntent.putExtra("UID", uid);
                    startActivity(wifiConfigIntent);
                }
            }else if( v == mBtnLogIn ){
                String uid = mEditUid.getText().toString().trim().toUpperCase();
                String password = mEditPassword.getText().toString().trim();
                if( password.length() == 0 ){
                    Toast.makeText(MainActivity.this, "Access password can't be empty", Toast.LENGTH_SHORT).show();
                }else {
                    // Show loading dialog
                    mConnectingDlg.setMessage("Connecting...");
                    mConnectingDlg.setCancelable(false);
                    mConnectingDlg.setCanceledOnTouchOutside(false);
                    mConnectingDlg.show();

                    // Register callback
                    EasyCamApi.getInstance().addLogInResultCallback(mLoginResultCallback=new EasyCamApi.LoginResultCallback() {
                        @Override
                        public void onLoginResult(int handle, int errorCode, int notificationToken, int isCharging, int batPercent) {
                            Log.d("Demo", "Login result,handle:"+handle+",errorCode:"+errorCode+",eventChannel:"+notificationToken);
                            if( errorCode != 0 ){
                                Message logInFailMsg = new Message();
                                logInFailMsg.what = MSG_ID_LOGIN_FAIL;
                                logInFailMsg.arg1 = errorCode;
                                mHandler.sendMessage(logInFailMsg);
                            }else{
                                mNotificationEventChannel = notificationToken;
                                mHandler.sendEmptyMessage(MSG_ID_LOGIN_SUCCESS);
                            }
                        }
                    }, mLoginSeq);
                    mSessionHandle = EasyCamApi.getInstance().logIn(uid, password, "192.168.1.255",
                            mLoginSeq, 1, 1,
                            EasyCamApi.CONNECT_TYPE_LAN|EasyCamApi.CONNECT_TYPE_P2P|EasyCamApi.CONNECT_TYPE_RELAY,
                            30);
                    mLoginSeq++;
                }
            }else if( v == mBtnLogout ){
                if( mSessionHandle >= 0 ){
                    EasyCamApi.getInstance().logOut(mSessionHandle);
                    if( mLoginResultCallback != null ){
                        EasyCamApi.getInstance().removeLogInResultCallback(mLoginResultCallback);
                    }

                    mBtnLogIn.setEnabled(true);
                    mBtnLogout.setEnabled(false);
                    mBtnCmdTest.setEnabled(false);
                    mBtnDpsSubsribe.setEnabled(false);
                    mBtnTalk.setEnabled(false);
                }
            }else if( v == mBtnCmdTest ){
                if( mSessionHandle >= 0 ){
                    isToCmdTestActivity = true;
                    Intent cmdTestIntent = new Intent(MainActivity.this, CmdTestActivity.class);
                    cmdTestIntent.putExtra("HANDLE", mSessionHandle);
                    startActivity(cmdTestIntent);
                }
            }else if( v == mBtnDpsSubsribe ){
                final String uid = mEditUid.getText().toString().trim().toUpperCase();
                String password = mEditPassword.getText().toString().trim();
                if( (uid.length()!=17) ){
                    Toast.makeText(MainActivity.this, "UID format error", Toast.LENGTH_SHORT).show();
                    return;
                }else if( password.length() == 0 ){
                    Toast.makeText(MainActivity.this, "Access password can't be empty", Toast.LENGTH_SHORT).show();
                    return;
                }

                if( mNotificationEventChannel == -1 ){
                    Toast.makeText(MainActivity.this, "Please login to the device fistly", Toast.LENGTH_SHORT).show();
                    return;
                }

                SharedPreferences sharedPreferences = getSharedPreferences("DPS_INFO", Activity.MODE_PRIVATE);
                final String DPS_token = sharedPreferences.getString("DPS_TOKEN", "");
                if( DPS_token.equals("") ){
                    Toast.makeText(MainActivity.this, "Please make sure your phone can connect to internet", Toast.LENGTH_SHORT).show();
                    return;
                }

                mConnectingDlg.setMessage("Subscribing message...");
                mConnectingDlg.setCancelable(false);
                mConnectingDlg.setCanceledOnTouchOutside(false);
                mConnectingDlg.show();

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        int ret = EasyCamApi.getInstance().subscribeMessage(uid, "WeHome", "DPS", DPS_token, "TestDeviceName", mNotificationEventChannel);
                        if( ret == 0 ){
                            mHandler.sendEmptyMessage(MSG_ID_SUBSCRIBE_MSG_SUCCESS);
                        }else{
                            mHandler.sendEmptyMessage(MSG_ID_SUBSCRIBE_MSG_FAIL);
                        }
                    }
                }).start();
            }else if( v == mBtnTalk ){
                if( !hasPermissions() ){
                    requestPermissions();
                }else{
                    // Start / Stop talk
                    if (!v.isSelected()) {
                        v.setSelected(true);
                        onTalkEnabled(true);
                    } else {
                        v.setSelected(false);
                        onTalkEnabled(false);
                    }
                }
            }
        }
    };

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull  String[] permissions, @NonNull  int[] grantResults) {
        if (requestCode == REQUEST_CODE_AUDIO_PERMISSION) {
            int granted = PackageManager.PERMISSION_GRANTED;
            for (int r : grantResults) {
                granted |= r;
            }
            if (granted == PackageManager.PERMISSION_GRANTED) {
                if (!mBtnTalk.isSelected()) {
                    mBtnTalk.setSelected(true);
                    onTalkEnabled(true);
                } else {
                    mBtnTalk.setSelected(false);
                    onTalkEnabled(false);
                }
            }
        }
    }

    @TargetApi(M)
    private void requestPermissions() {
        final String[] permissions = new String[]{ RECORD_AUDIO};

        boolean showRationale = false;
        for (String perm : permissions) {
            showRationale |= shouldShowRequestPermissionRationale(perm);
        }
        if (!showRationale) {
            requestPermissions(permissions, REQUEST_CODE_AUDIO_PERMISSION);
            return;
        }

        new AlertDialog.Builder(this)
                .setMessage("Need permission to record audio")
                .setCancelable(false)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        requestPermissions(permissions, REQUEST_CODE_AUDIO_PERMISSION);
                    }
                })
                .setNegativeButton(android.R.string.cancel, null)
                .create()
                .show();
    }

    private boolean hasPermissions() {
        PackageManager pm = getPackageManager();
        String packageName = getPackageName();
        int granted = pm.checkPermission(RECORD_AUDIO, packageName);
        return granted == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    protected void onPause() {
        super.onPause();

        String uid = mEditUid.getText().toString();
        if( uid.length()==17 ) {
            SharedPreferences sp = getSharedPreferences("DEMO", MODE_PRIVATE);
            SharedPreferences.Editor editor = sp.edit();
            editor.putString("UID", uid);
            editor.apply();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        isToCmdTestActivity = false;
    }

    @Override
    protected void onStop() {
        super.onStop();
        if( !isToCmdTestActivity ){
            EasyCamApi.getInstance().logOut(mSessionHandle);
            mBtnLogIn.setEnabled(true);
            mBtnLogout.setEnabled(false);
            mBtnCmdTest.setEnabled(false);
            mBtnDpsSubsribe.setEnabled(false);
            mBtnTalk.setEnabled(false);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if( mAudioTrack != null ){
            mAudioTrack.stop();
            mAudioTrack.release();
        }
    }

    private EasyCamApi.CommandResultCallback mCommandResultCallback;

    protected void onTalkEnabled(boolean enabled) {

        if( enabled){
            mConnectingDlg.setMessage("Opening talk...");
        }else{
            mConnectingDlg.setMessage("Closing talk...");

            // Stop audio thread
            mIsAudioThreadRun = false;
            if( mAudioRecordThread != null ){
                try {
                    mAudioRecordThread.join();
                    mAudioRecordThread = null;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        mConnectingDlg.setCancelable(false);
        mConnectingDlg.setCanceledOnTouchOutside(false);
        mConnectingDlg.show();

        String cmdString;
        if( enabled ){
            cmdString = "{\"cmdId\":256}";
        }else{
            cmdString = "{\"cmdId\":257}";
        }

        if( mCommandResultCallback != null ){
            EasyCamApi.getInstance().removeCommandResultCallback(mCommandResultCallback);
        }
        EasyCamApi.getInstance().addCommandResultCallback(mCommandResultCallback=new EasyCamApi.CommandResultCallback() {
            @Override
            public void onCommandResult(int handle, int errCode, String data, int seq) {
                if( errCode == 0 ){
                    // Parse result(openTalkResult)
                    try {
                        JSONObject jsonObject = new JSONObject(data);
                        int cmdId = jsonObject.getInt("cmdId");
                        if( cmdId == 256 ) {
                            if (jsonObject.has("openTalkResult")) {
                                int openTalkResult = jsonObject.getInt("openTalkResult");
                                if (openTalkResult == 1) {
                                    mHandler.sendEmptyMessage(MSG_ID_START_TALK_SUCCESS);
                                } else {
                                    mHandler.sendEmptyMessage(MSG_ID_START_TALK_OCCUPIED);
                                }
                            } else {
                                // For some old firmware version which doesn't support openTalkResult field.
                                mHandler.sendEmptyMessage(MSG_ID_START_TALK_SUCCESS);
                            }
                        }else{
                            mHandler.sendEmptyMessage(MSG_ID_CLOSE_TALK_SUCCESS);
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }

                }else{
                    Message cmdExecErrMsg = new Message();
                    cmdExecErrMsg.what = MSG_ID_START_TALK_FAILED;
                    cmdExecErrMsg.arg1 = errCode;
                    mHandler.sendMessage(cmdExecErrMsg);
                }
            }
        }, mCmdSeqNo);
        EasyCamApi.getInstance().sendCommand(mSessionHandle, cmdString, cmdString.length()+1, mCmdSeqNo);

        mCmdSeqNo++;
    }

    private class VideoQueueProcess extends Thread {

        boolean interrupt;

        VideoQueueProcess(String threadName) {
            super(threadName);
        }

        @Override
        public void run() {
            interrupt = false;
            while (!interrupt && !isInterrupted()) {
                try {
                    if( mStreamViewer!=null && mStreamViewer.isSurfaceReady() ) {
                        mVideoFrameQueue.take();
                    }
                } catch (InterruptedException e) {
                    break;
                }
            }
        }
    }

    private class AudioQueueProcess extends Thread {

        boolean interrupt;

        AudioQueueProcess(String threadName) {
            super(threadName);
        }

        @Override
        public void run() {
            interrupt = false;
            while (!interrupt && !isInterrupted()) {
                try {
                    mAudioFrameQueue.take();
                } catch (InterruptedException e) {
                    break;
                }
            }
        }
    }


    private class IECVideoFrameQueue extends ECVideoFrameQueue {

        private boolean mGetVideoPayloadType;
        private boolean mNeedWaitIDR = true;

        IECVideoFrameQueue(int capacity) {
            super(capacity);
        }

        @Override
        protected boolean onTake(byte[] data, int dataLen, int payloadType, long timestamp,
                                 int frameType, int videoWidth, int videoHeight, int wifiQuality, int playbackSession) {

            // get video payload type information
            if (!mGetVideoPayloadType) {
                mGetVideoPayloadType = true;

                // setting to monitor control information
                mVideoWidth = videoWidth;
                mVideoHeight = videoHeight;
            }

            if( mNeedWaitIDR ){
                if( frameType != EasyCamApi.EasyCamAVStreamCallback.FRAME_TYPE_I ){
                    Log.d("DEMO", "Wait IDR frame");
                    return true;
                }else{
                    mNeedWaitIDR = false;
                }
            }

            // Surface view 准备好的时候再解码描绘
            mStreamViewer.setForceSoftwareDecode(true);

            if( mStreamViewer.isSurfaceReady() ){
                if( mStreamViewer.isHWDecoderSupported() ){
                    if( !mStreamViewer.hardDecodeImage(data, dataLen) ){
                        Log.w("DEMO", "Change to software decoder");
                        mStreamViewer.setForceSoftwareDecode(true);
                    }
                }
                if( !mStreamViewer.isHWDecoderSupported() ){
                    // decode
                    mH264ByteBuffer.rewind();
                    mH264ByteBuffer.put(data, 0, dataLen);
                    mH264Decoder.consumeNalUnitsFromDirectBuffer(mH264ByteBuffer, dataLen, timestamp);

                    int width = mH264Decoder.getWidth();
                    int height = mH264Decoder.getHeight();
                    if ( (mRGBByteBuffer==null) || (width != mVideoWidth) ||
                            (height != mVideoHeight) ) {
                        // setting to monitor control information
                        mVideoWidth = width;
                        mVideoHeight = height;

                        // 分配输出缓冲区
                        if( mRGBByteBuffer != null ){
                            mRGBByteBuffer = null;
                        }
                        mRGBByteBuffer = ByteBuffer.allocateDirect(mH264Decoder.getOutputByteSize());
                    }

                    mRGBByteBuffer.rewind();
                    mH264Decoder.decodeFrameToDirectBuffer(mRGBByteBuffer);

                    // draw image
                    mStreamViewer.drawImage(mRGBByteBuffer, width, height);
                }
            }

            return true;
        }
    }

    private class IECAudioFrameQueue extends ECAudioFrameQueue {

        private boolean mNotSupported;

        IECAudioFrameQueue(int capacity) {
            super(capacity);
        }

        private void processPCMAudioData(byte[] data, int dataLen, int sampleRate, int channelCount) {

            if( mSessionHandle >= 0 ) {
                // Play audio stream
                mAudioTrack.write(data, 0, dataLen);
            }
        }

        @Override
        protected boolean onTake(byte[] data, int dataLen, int payloadType, long timestamp, int playbackSession) {

            if( isFinishing() ){
                return true;
            }

            // process audio frame
            if (EasyCamApi.PayloadType.PAYLOAD_TYPE_AUDIO_PCM == payloadType) {
                processPCMAudioData(data, dataLen, 16000, 1);
            } else {
                // not support audio payload type to play, show dialog
                if (!mNotSupported) {
                    mNotSupported = true;
                }
            }

            return true;
        }
    }

    private Runnable RecordAudioRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

                mAudioBufSize = AudioRecord.getMinBufferSize(16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
                mRecorder = new AudioRecord(MediaRecorder.AudioSource.MIC, 16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, mAudioBufSize * 2);

                int bytesRecord;
                //int bufferSize = 320;
                byte[] tempBuffer = new byte[640];

                mRecorder.startRecording();

                while (mIsAudioThreadRun) {
                    if (null != mRecorder) {
                        bytesRecord = mRecorder.read(tempBuffer, 0, tempBuffer.length);
                        if (bytesRecord == AudioRecord.ERROR_INVALID_OPERATION || bytesRecord == AudioRecord.ERROR_BAD_VALUE) {
                            continue;
                        }
                        if (bytesRecord != 0 && bytesRecord != -1) {
                            // seq is not in using
                            EasyCamApi.getInstance().sendTalkData(mSessionHandle, tempBuffer, bytesRecord, EasyCamApi.PayloadType.PAYLOAD_TYPE_AUDIO_PCM, 0);
                        } else {
                            break;
                        }
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }

            if (mRecorder != null) {
                if (mRecorder.getState() == AudioRecord.STATE_INITIALIZED) {
                    mRecorder.stop();
                }
                if (mRecorder != null) {
                    mRecorder.release();
                }
            }
        }
    };
}
