package com.linkwil.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import com.dps.ppcs_api.DPS_API;

public class Alarm_Receiver extends BroadcastReceiver {
	@Override
    public void onReceive(Context context, Intent intent) {

        Bundle bData = intent.getExtras();
        if (bData != null) {
            String msg = (String )bData.get("msg");
            if ( msg!=null && msg.equals("timer")) {
                int[] time_Sec = new int[1];
                int ret = DPS_API.DPS_GetLastAliveTime(time_Sec);
                if (ret == DPS_API.ERROR_DPS_Successful) {
                    if (time_Sec[0] > 40) {
                        DPS_API.DPS_DeInitialize();
                    }
                }
            }
        }
    }
}