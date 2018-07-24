package com.linkwil;

import android.app.Application;

import com.linkwil.easycamsdk.EasyCamApi;

/**
 * Created by xiao on 2016/12/7.
 */

public class DemoApplication extends Application{
    public void onCreate(){
        super.onCreate();

        EasyCamApi.getInstance().init();
    }
}
