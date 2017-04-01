package ican.ytx.com.mediacodectest.hybridplayer;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import ican.ytx.com.mediacodectest.R;
import ican.ytx.com.mediacodectest.media.player.view.YtxVideoView;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private String [] filePath = new String[]{
            "/storage/emulated/0/titanic.mkv",
            "/storage/emulated/0/video2.mp4",
            "/storage/emulated/0/gqfc07.ts",
            "/storage/emulated/0/test_file/x7_11.mkv",
    };

    YtxVideoView ytxVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ytxVideoView = (YtxVideoView) findViewById(R.id.ytxVideoView);
        findViewById(R.id.bt).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ytxVideoView.setVideoPath(filePath[0]);
                ytxVideoView.start();
            }
        });

    }


    @Override
    protected void onResume() {
        super.onResume();

    }

}
