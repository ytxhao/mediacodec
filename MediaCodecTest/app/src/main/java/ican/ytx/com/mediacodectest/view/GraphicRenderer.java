package ican.ytx.com.mediacodectest.view;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;

import java.nio.IntBuffer;
import java.util.Vector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Administrator on 2017/1/22.
 */

public class GraphicRenderer implements GLSurfaceView.Renderer {

    public int mScreenWidth;
    public int mScreenHeight;

    public RendererUtils2.RenderContext renderContext;
    public VideoGlSurfaceView mGLSurfaceView;
    public Image image;

    public VideoFrame mVideoFrame;

    public final Vector<Runnable> queue = new Vector<Runnable>();

    private static volatile boolean mIsNativeInitialized = false;

    private static void initNativeOnce() {
        synchronized (GraphicRenderer.class) {
            if (!mIsNativeInitialized) {
                mIsNativeInitialized = true;
            }
        }
    }

    public GraphicRenderer() {
        initNativeOnce();
    }

    public void setImage(Image image) {
        this.image = image;
    }

    public void setVideoFrame(VideoFrame mVideoFrame) {
        this.mVideoFrame = mVideoFrame;
    }

    public void setGlSurfaceView(VideoGlSurfaceView mGLSurfaceView) {
        this.mGLSurfaceView = mGLSurfaceView;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glEnable(GLES20.GL_TEXTURE_2D);
        GLES20.glGetError();
        renderContext = RendererUtils2.createProgram();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mScreenWidth = width;
        mScreenHeight = height;
        // Set viewport
        GLES20.glViewport(0, 0,mScreenWidth, mScreenHeight);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Runnable r = null;
        synchronized (queue) {
            if (!queue.isEmpty()) {
                r = queue.remove(0);
            }
        }
        if (r != null) {
            r.run();
        }

        // if (!queue.isEmpty()) {
        //mGLSurfaceView.requestRender();
        //  }
        synchronized(this) {
            RendererUtils2.renderBackground();
            //mGLSurfaceView.drawFrame();
            if (mVideoFrame != null) {
                RendererUtils2.renderTexture(renderContext, mVideoFrame, mScreenWidth, mScreenHeight);
            }
        }
    }


}




