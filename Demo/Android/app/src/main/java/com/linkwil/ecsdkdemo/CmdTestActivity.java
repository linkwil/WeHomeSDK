package com.linkwil.ecsdkdemo;

import android.app.ProgressDialog;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.linkwil.easycamsdk.EasyCamApi;

import java.lang.ref.WeakReference;

public class CmdTestActivity extends AppCompatActivity {

    private final int MSG_ID_CMD_EXEC_SUCCESS = 0;
    private final int MSG_ID_CMD_EXEC_FAIL = 1;

    private Button mBtnExecCmd;
    private EditText mEditCmdString;
    private TextView mTextCmdResult;

    private ProgressDialog mProgressDlg;

    private int mSessionHandle;
    private int mCmdSeqNo = 0;
    private String mCmdResultString;

    private EasyCamApi.CommandResultCallback mCommandResultCallback;

    private MyHandler mHandler = new MyHandler(this);
    private static class MyHandler extends Handler {
        private WeakReference<CmdTestActivity> reference;

        private MyHandler(CmdTestActivity activity) {
            reference = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            CmdTestActivity activity = reference.get();
            if( activity != null )
                activity.handleMsg(msg);
        }
    }

    public void handleMsg(Message msg) {
        if (!isFinishing()) {
            if (msg.what == MSG_ID_CMD_EXEC_SUCCESS) {
                mProgressDlg.dismiss();

                mTextCmdResult.setText(mCmdResultString);
            }else if( msg.what == MSG_ID_CMD_EXEC_FAIL ){
                mProgressDlg.dismiss();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cmd_test);

        mBtnExecCmd = findViewById(R.id.btn_exec_cmd);
        mEditCmdString = findViewById(R.id.et_cmd_string);
        mTextCmdResult = findViewById(R.id.tv_cmd_result);

        mProgressDlg = new ProgressDialog(this);
        mProgressDlg.setMessage("Connecting...");

        mEditCmdString.setText("{\"cmdId\":512}");

        mSessionHandle = getIntent().getIntExtra("HANDLE", -1);

        mBtnExecCmd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String cmdString = mEditCmdString.getText().toString().trim();

                mProgressDlg.setCancelable(false);
                mProgressDlg.setCanceledOnTouchOutside(false);
                mProgressDlg.show();

                if( mCommandResultCallback != null ){
                    EasyCamApi.getInstance().removeCommandResultCallback(mCommandResultCallback);
                }
                EasyCamApi.getInstance().addCommandResultCallback(mCommandResultCallback=new EasyCamApi.CommandResultCallback() {
                    @Override
                    public void onCommandResult(int handle, int errCode, String data, int seq) {
                        if( errCode == 0 ){
                            mCmdResultString = data;
                            mHandler.sendEmptyMessage(MSG_ID_CMD_EXEC_SUCCESS);
                        }else{
                            Message cmdExecErrMsg = new Message();
                            cmdExecErrMsg.what = MSG_ID_CMD_EXEC_FAIL;
                            cmdExecErrMsg.arg1 = errCode;
                            mHandler.sendMessage(cmdExecErrMsg);
                        }
                    }
                }, mCmdSeqNo);
                EasyCamApi.getInstance().sendCommand(mSessionHandle, cmdString, cmdString.length()+1, mCmdSeqNo);

                mCmdSeqNo++;
            }
        });
    }
}
