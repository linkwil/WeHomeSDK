package com.linkwil.ecsdkdemo;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.linkwil.wificonfig.LinkWiFiConfigApi;

public class WifiConfigActivity extends AppCompatActivity {

    private EditText etWiFiSsid;
    private EditText etWiFiPassword;
    private EditText etDevPassword;
    private Button btnStartConfig;
    private Button btnStopConfig;

    private String mUid;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wifi_config);

        etWiFiSsid = findViewById(R.id.et_wifi_ssid);
        etWiFiPassword = findViewById(R.id.et_wifi_password);
        etDevPassword = findViewById(R.id.et_dev_password);
        btnStartConfig = findViewById(R.id.btn_start_wifi_config);
        btnStopConfig = findViewById(R.id.btn_stop_wifi_config);

        mUid = getIntent().getStringExtra("UID");

        btnStartConfig.setOnClickListener(mOnClickListener);
        btnStopConfig.setOnClickListener(mOnClickListener);

        btnStartConfig.setEnabled(true);
        btnStopConfig.setEnabled(false);

        String ssid;
        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        if( wifiManager != null ) {
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if ((wifiInfo != null) && (wifiManager.isWifiEnabled()) && isWifiConnected(this)) {
                ssid = wifiInfo.getSSID();
                ssid = ssid.replace("\"", "");
                if (ssid != null) {
                    etWiFiSsid.setText(ssid);
                } else {
                    etWiFiSsid.setText("");
                }
            } else {
                etWiFiSsid.setText("");
            }
        }else{
            etWiFiSsid.setText("");
        }
    }

    private View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if( v == btnStartConfig ){
                String wifiSsid = etWiFiSsid.getText().toString();
                String wifiPassword = etWiFiPassword.getText().toString();
                String devPassword = etDevPassword.getText().toString();
                if( wifiSsid.length() == 0 ){
                    Toast.makeText(WifiConfigActivity.this, "Wi-Fi ssid can't be empty", Toast.LENGTH_SHORT).show();
                }else if( devPassword.length() == 0 ){
                    Toast.makeText(WifiConfigActivity.this, "Device access password can't be empty", Toast.LENGTH_SHORT).show();
                }else {
                    LinkWiFiConfigApi.getInstance().startSmartConfig(WifiConfigActivity.this, wifiSsid, wifiPassword, devPassword);
                    btnStopConfig.setEnabled(true);
                    btnStartConfig.setEnabled(false);
                }
            }else if( v == btnStopConfig ){
                LinkWiFiConfigApi.getInstance().stopSmartConfig();
                btnStartConfig.setEnabled(true);
                btnStopConfig.setEnabled(false);
            }
        }
    };

    public boolean isWifiConnected(Context context)
    {
        ConnectivityManager connectivityManager = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if( connectivityManager != null ) {
            NetworkInfo wifiNetworkInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
            if (wifiNetworkInfo.isConnected()) {
                return true;
            }
        }

        return false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        LinkWiFiConfigApi.getInstance().stopSmartConfig();
    }
}
