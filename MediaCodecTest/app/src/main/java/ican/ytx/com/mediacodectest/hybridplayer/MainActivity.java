package ican.ytx.com.mediacodectest.hybridplayer;

import android.Manifest;
import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import java.util.List;

import ican.ytx.com.mediacodectest.R;
import ican.ytx.com.mediacodectest.media.player.permission.PermissionRequestListener;
import ican.ytx.com.mediacodectest.media.player.permission.PermissionUtil;
import ican.ytx.com.mediacodectest.media.player.view.YtxVideoView;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private String [] filePath = new String[]{
            "/storage/emulated/0/media_decryptd_2018.ts",
            "/storage/emulated/0/media_decryptd_nomarl.ts",
            "/storage/emulated/0/media_decryptd_nomarl2.ts",
            "/storage/emulated/0/media_decryptd_nomarl3.ts",
            "/storage/emulated/0/titanic.mkv",
            "/storage/emulated/0/video2.mp4",
            "/storage/emulated/0/gqfc07.ts",
            "/storage/emulated/0/testfile.mp4",
            "/storage/emulated/0/SkyF1.ts",
            "/storage/emulated/0/test_file/x7_11.mkv",
    };

    YtxVideoView ytxVideoView;
    private String[] permissionArray = new String[]{Manifest.permission.READ_PHONE_STATE,
            Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE};
    private String[] permissionStorageArray = new String[]{
            Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        PermissionUtil.newInstance(this).requestPermission(this, PermissionUtil.PERMISSION_REQUEST_CODE_MAIN_INTERNATIONAL,
                permissionRequestListener, permissionStorageArray);
        ytxVideoView = (YtxVideoView) findViewById(R.id.ytxVideoView);
        findViewById(R.id.bt).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ytxVideoView.setVideoPath(filePath[0]);
                ytxVideoView.start();
            }
        });

        findViewById(R.id.btSave).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

    }


    @Override
    protected void onResume() {
        super.onResume();

    }


    private PermissionRequestListener permissionRequestListener = new PermissionRequestListener() {
        @Override
        public void onPermissionGranted(int requestCode) {
            if(requestCode == PermissionUtil.PERMISSION_REQUEST_CODE_MAIN_INTERNAL) {

            }
        }

        @Override
        public void onPermissionsDenied(int requestCode, List<String> deniedList) {
            if ((deniedList.contains(Manifest.permission.READ_EXTERNAL_STORAGE) &&
                    !PermissionUtil.permissionPermanentlyDenied(MainActivity.this, Manifest.permission.READ_EXTERNAL_STORAGE)) ||
                    (deniedList.contains(Manifest.permission.WRITE_EXTERNAL_STORAGE) &&
                            !PermissionUtil.permissionPermanentlyDenied(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE))) {

            }

        }
    };
}
