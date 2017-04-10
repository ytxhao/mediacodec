package ican.ytx.com.mediacodectest.media.player.render;

import android.graphics.SurfaceTexture;

import ican.ytx.com.mediacodectest.media.player.pragma.YtxLog;

/**
 * Created by Administrator on 2017/4/10.
 */

public class MSurfaceTexture extends SurfaceTexture implements SurfaceTexture.OnFrameAvailableListener{

    private static final String TAG = "MSurfaceTexture";
    volatile boolean updateSurface = false;

    public MSurfaceTexture(int texName) {
        super(texName);
    }

    public MSurfaceTexture(int texName, boolean singleBufferMode) {
        super(texName, singleBufferMode);
    }


    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        updateSurface = true;
        YtxLog.d(TAG,"onFrameAvailable");
    }


    public void updateSurfaceNative(){
        YtxLog.d(TAG,"updateSurfaceNative");
        synchronized (this) {
            if (updateSurface) {
                updateTexImage();
                updateSurface = false;
            }
        }
    }

}
