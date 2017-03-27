package ican.ytx.com.mediacodectest.view;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.Vector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Administrator on 2017/1/22.
 */

public class GraphicRenderer implements GLSurfaceView.Renderer {


    public int mMaxTextureSize;
    public int viewWidth;
    public int viewHeight;

    public RendererUtils.RenderContext renderContext;
    public VideoGlSurfaceView mGLSurfaceView;
    public Image image;

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

    public void setGlSurfaceView(VideoGlSurfaceView mGLSurfaceView) {
        this.mGLSurfaceView = mGLSurfaceView;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glEnable(GLES20.GL_TEXTURE_2D);
        IntBuffer buffer = IntBuffer.allocate(1);
        GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, buffer);
        mMaxTextureSize = buffer.get(0);
        GLES20.glGetError();
        renderContext = RendererUtils.createProgram();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        viewWidth = width;
        viewHeight = height;
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
        mGLSurfaceView.requestRender();
        //  }
        RendererUtils.renderBackground();
        mGLSurfaceView.drawFrame();

        if (image != null) {
            // buildAnimal();
            //  setRenderMatrix(image.width(), image.height());
            RendererUtils.renderTexture(renderContext, image.texture(),
                    viewWidth, viewHeight);
        }
    }


}




