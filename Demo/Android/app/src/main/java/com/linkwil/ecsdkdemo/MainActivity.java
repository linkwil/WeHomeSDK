package com.linkwil.ecsdkdemo;

import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.Message;
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
import com.linkwil.util.ECAudioFrameQueue;
import com.linkwil.util.ECVideoFrameQueue;
import com.linkwil.util.StreamView;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {

    private final int LIVE_VIDEO_FRAME_BUF_SIZE = 60;
    private final int LIVE_AUDIO_FRAME_BUF_SIZE = 255;

    private Button mBtnWiFiConfig;
    private EditText mEditUid;
    private EditText mEditPassword;
    private Button mBtnLogIn;
    private Button mBtnLogout;
    private Button mBtnCmdTest;
    private FrameLayout mStreamViewerContainer;
    private StreamView mStreamViewer;

    private int mVideoWidth, mVideoHeight;

    private H264Decoder mH264Decoder;
    private final int MAX_VIDEO_FRAME_SIZE = 512*1024;
    private ByteBuffer mH264ByteBuffer;
    private ByteBuffer mRGBByteBuffer;

    private int mLoginSeq = 0;
    private int mSessionHandle = -1;

    private final int MSG_ID_LOGIN_FAIL = 0;
    private final int MSG_ID_LOGIN_SUCCESS = 1;

    private EasyCamApi.LoginResultCallback mLoginResultCallback;

    private MyHandler mHandler = new MyHandler(this);

    private ProgressDialog mConnectingDlg;

    private IECVideoFrameQueue mVideoFrameQueue = new IECVideoFrameQueue(LIVE_VIDEO_FRAME_BUF_SIZE);
    private VideoQueueProcess mVideoQueueProcess;
    private IECAudioFrameQueue mAudioFrameQueue = new IECAudioFrameQueue(LIVE_AUDIO_FRAME_BUF_SIZE);
    private AudioQueueProcess mAudioQueueProcess;

    private int bufSize = AudioTrack.getMinBufferSize(16000, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
    private AudioTrack mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 16000, AudioFormat.CHANNEL_OUT_MONO,
            AudioFormat.ENCODING_PCM_16BIT, bufSize*2, AudioTrack.MODE_STREAM);

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

                mBtnLogout.setEnabled(true);
                mBtnCmdTest.setEnabled(true);
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
        mStreamViewerContainer = findViewById(R.id.layout_player_container);

        mConnectingDlg = new ProgressDialog(this);

        mBtnWiFiConfig.setOnClickListener(mOnClickListener);
        mBtnLogIn.setOnClickListener(mOnClickListener);
        mBtnLogout.setOnClickListener(mOnClickListener);
        mBtnCmdTest.setOnClickListener(mOnClickListener);

        mBtnLogout.setEnabled(false);
        mBtnCmdTest.setEnabled(false);

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
                char ori[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
                return ori;
            }

            @Override
            protected char[] getReplacement() {
                char replace[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
                return replace;
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
    }

    View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if(v==mBtnWiFiConfig){
                String uid = mEditUid.getText().toString().trim().toUpperCase();
                if( uid.length() != 17 ){
                    Toast.makeText(MainActivity.this, "UID format error", Toast.LENGTH_SHORT).show();
                }else {
                    Intent wifiConfigIntent = new Intent(MainActivity.this, WifiConfigActivity.class);
                    wifiConfigIntent.putExtra("UID", uid);
                    startActivity(wifiConfigIntent);
                }
            }else if( v == mBtnLogIn ){
                String uid = mEditUid.getText().toString().trim().toUpperCase();
                String password = mEditPassword.getText().toString().trim();
                if( (uid.length()!=17) ){
                    Toast.makeText(MainActivity.this, "UID format error", Toast.LENGTH_SHORT).show();
                }else if( password.length() == 0 ){
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
                }
            }else if( v == mBtnCmdTest ){
                if( mSessionHandle >= 0 ){
                    Intent cmdTestIntent = new Intent(MainActivity.this, CmdTestActivity.class);
                    cmdTestIntent.putExtra("HANDLE", mSessionHandle);
                    startActivity(cmdTestIntent);
                }
            }
        }
    };

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
    protected void onDestroy() {
        super.onDestroy();

        if( mAudioTrack != null ){
            mAudioTrack.stop();
            mAudioTrack.release();
        }
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

}
