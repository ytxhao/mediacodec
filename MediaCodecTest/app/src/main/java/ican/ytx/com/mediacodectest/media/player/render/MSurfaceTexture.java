package ican.ytx.com.mediacodectest.media.player.render;

import android.graphics.SurfaceTexture;

import ican.ytx.com.mediacodectest.media.player.pragma.IMediaPlayer;
import ican.ytx.com.mediacodectest.media.player.pragma.YtxLog;

/**
 * Created by Administrator on 2017/4/10.
 */

public class MSurfaceTexture extends SurfaceTexture implements SurfaceTexture.OnFrameAvailableListener{

    private static final String TAG = "MSurfaceTexture";
    volatile boolean updateSurface = false;
    private IMediaPlayer mMediaPlayer;

    public MSurfaceTexture(int texName,IMediaPlayer mMediaPlayer) {
        super(texName);
        this.mMediaPlayer = mMediaPlayer;
    }

    public MSurfaceTexture(int texName, boolean singleBufferMode) {
        super(texName, singleBufferMode);
    }


    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        YtxLog.d(TAG,"onFrameAvailable for test");
        updateSurface = true;
        requestRenderer();
    }


    public void updateSurfaceNative(){
        YtxLog.d(TAG,"updateSurfaceNative for test");
        synchronized (this) {
            if (updateSurface) {
                updateTexImage();
                updateSurface = false;
            }
        }
    }

    private void requestRenderer(){
        mMediaPlayer.requestRenderer();
    }

}
