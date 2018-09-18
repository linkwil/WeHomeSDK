package com.linkwil.service;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.IBinder;
import android.os.SystemClock;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;
import android.util.Base64;
import android.util.Log;
import android.widget.RemoteViews;
import android.widget.Toast;

import com.dps.ppcs_api.DPS_API;
import com.linkwil.ecsdkdemo.R;

import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

import static com.dps.ppcs_api.DPS_API.ERROR_DPS_NotInitialized;

public class DemoService extends Service {

    private boolean mHasDpsInited = false;
    private String DPS_token;
    private Boolean RUN_THREAD = false;

    private NotificationChannel mDemoNotificationChannel;
    private String mDemoNotificationId;

    @Override
    public void onCreate() {
        super.onCreate();

        // DPS初始化比较耗时，放到线程中初始化，以免卡住后台服务创建
        if( !mHasDpsInited ) {
            new Thread(DPSInitRunnable).start();
            mHasDpsInited = true;
        }

        mDemoNotificationId = getPackageName()+"_notification";
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

            mDemoNotificationChannel = new NotificationChannel(mDemoNotificationId, "DemoNotification", NotificationManager.IMPORTANCE_DEFAULT);
            mDemoNotificationChannel.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC);
            if( notificationManager != null ) {
                notificationManager.createNotificationChannel(mDemoNotificationChannel);
            }
        }
    }

    private void CancelAlarmTimer(){
        Intent intent = new Intent(this, Alarm_Receiver.class);
        PendingIntent pintent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        AlarmManager manager = (AlarmManager) (this.getSystemService(Context.ALARM_SERVICE));
        if( manager != null ) {
            manager.cancel(pintent);
        }
    }

    public void SetAlarmTimer() {
        Intent intent = new Intent(this, Alarm_Receiver.class);
        intent.putExtra("msg", "timer");
        PendingIntent pintent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        AlarmManager manager = (AlarmManager) (this.getSystemService(Context.ALARM_SERVICE));
        if( manager != null ) {
            manager.cancel(pintent);
            manager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                    SystemClock.elapsedRealtime() + 60 * 1000,  // 闹钟首次执行时间
                    5 * 60 * 1000,  // 闹钟重复间隔(5min)
                    pintent);
        }
    }

    private Runnable DPSInitRunnable = new Runnable() {
        @Override
        public void run() {
            int initRet = 0;

            //// Initialize DPS:
            initRet = DPS_API.DPS_Initialize("119.23.219.190",32750, "tB7mesK842996reU", 0);
            if (initRet == DPS_API.ERROR_DPS_Successful || initRet == DPS_API.ERROR_DPS_AlreadyInitialized){
                Log.d("LinkBell", "DPS_Initialize success");

                new Thread(GetDPSTokenRunnable).start();

                RUN_THREAD = true;
                new Thread(DPS_RecvNotify).start();
            }
        }
    };

    private Runnable GetDPSTokenRunnable = new Runnable() {

        int ret = 0;
        byte[] RecvBuf = new byte[48];

        @Override
        public void run() {

            Arrays.fill(RecvBuf, (byte) 0);

            SharedPreferences sharedPreferences = getSharedPreferences("DPS_INFO", Activity.MODE_PRIVATE);
            DPS_token = sharedPreferences.getString("DPS_TOKEN", "");
            Log.d("WeHomeDemo", "DPS_Token:"+DPS_token);
            if( DPS_token.length() == 0 ) {
                // 开始获取DPS Token
                Log.d("LinkBell", "DPS token is empty, start acquire...");
                ret = DPS_API.DPS_TokenAcquire(RecvBuf, 48);
                Log.d("LinkBell", "DPS_TokenAcquire ret:"+ret);
                if( ret >= 0 ){
                    DPS_token = new String(Arrays.copyOf(RecvBuf, 32));
                    Log.d("LinkBell", "Acquire DPS token:"+DPS_token);

                    SharedPreferences.Editor editor = sharedPreferences.edit();
                    editor.putString("DPS_TOKEN", DPS_token);
                    editor.apply();
                }
            }
        }
    };

    private Runnable DPS_RecvNotify = new Runnable() {
        public void run() {
            try {
                byte[] Buf = new byte[1408];
                int[] Size = new int[1];
                int ret;

                while (RUN_THREAD) {
                    Arrays.fill(Buf, (byte) 0);
                    Size[0] = 1408;

                    if ( (DPS_token==null) || (DPS_token.length()==0) ) {
                        // 等待DPS_Token准备好
                        SharedPreferences settings = getSharedPreferences("DPS_INFO", 0);
                        DPS_token = settings.getString("DPS_TOKEN", "");
                        if (DPS_token.length() == 0) {
                            Thread.sleep(1000);
                            continue;
                        }
                    }

                    SetAlarmTimer();
                    ret = DPS_API.DPS_RecvNotify(DPS_token, Buf, Size, 86400 * 1000); // 24h
                    CancelAlarmTimer();

                    if (ret < 0) {
                        if (ret == DPS_API.ERROR_DPS_OnRecvNotify) {
                            RUN_THREAD = false;
                        } else if (ret == ERROR_DPS_NotInitialized || ret == DPS_API.ERROR_DPS_FailedToRecvData || ret == DPS_API.ERROR_DPS_FailedToConnectServer) {
                            DPS_API.DPS_DeInitialize();
                            ret = DPS_API.DPS_Initialize("119.23.219.190",32750, "tB7mesK842996reU", 0);;
                            //If you didn't do writeToFile, the sleeping period should be 500ms~1000ms
                            Thread.sleep(20000);
                        }
                        continue;
                    }
                    String recvStr = new String(Arrays.copyOf(Buf, Size[0]));
                    Log.d("DPS", "Recv notification:"+recvStr);
                    try {
                        JSONObject data = new JSONObject(recvStr);
                        String title = data.getString("title");
                        String content = data.getString("content");
                        String payLoad = new String(Base64.decode(
                                data.getJSONObject("custom_content").getString("payload"),
                                Base64.DEFAULT
                        ));

                        //onRecvPushMsg(title, content, payLoad);
                        showNotification(title, content);

                    } catch (Exception e) {
                        Log.e("DPS", "Error json parser:"+e.toString());
                    }
                }
                RUN_THREAD = false;
            } catch (Exception e) {
                Log.e("LinkBell", "Recv notify exception:"+e.getMessage());
                e.printStackTrace();
            }
            DPS_API.DPS_DeInitialize();
        }
    };

    void showNotification(String title, String content){

        RemoteViews notificationRemoteView = new RemoteViews(getPackageName(), R.layout.notification_remote_view);
        notificationRemoteView.setTextViewText(R.id.tv_notification_remoteview_title, title);
        notificationRemoteView.setTextViewText(R.id.tv_notification_remoteview_content, content);

        NotificationCompat.Builder motionNotification = new NotificationCompat.Builder(this, mDemoNotificationId)
                .setSmallIcon(R.drawable.ic_launcher_background)
                .setTicker("Demo notification")
                .setContentTitle(title)
                .setContentText(content)
                .setCustomContentView(notificationRemoteView)
                .setDefaults(NotificationCompat.DEFAULT_VIBRATE|NotificationCompat.DEFAULT_SOUND)
                .setAutoCancel(true)
                .setPriority(Notification.PRIORITY_MAX);

        NotificationManager manager = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
        if( manager != null ) {
            manager.notify((int) System.currentTimeMillis(), motionNotification.build());
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
