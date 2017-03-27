package ican.ytx.com.mediacodectest;

import android.graphics.Picture;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import ican.ytx.com.mediacodectest.view.VideoGlSurfaceView;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private String [] filePath = new String[]{
            "/storage/emulated/0/titanic.mkv",
            "/storage/emulated/0/video2.mp4",
            "/storage/emulated/0/gqfc07.ts",
            "/storage/emulated/0/test_file/x7_11.mkv",
    };


    VideoGlSurfaceView mVideoGlSurfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mVideoGlSurfaceView = (VideoGlSurfaceView) findViewById(R.id.surfaceView);

    }


    @Override
    protected void onResume() {
        super.onResume();
        mVideoGlSurfaceView.onResume();
    }
}
