package ican.ytx.com.mediacodectest.media.player.render;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.Interpolator;


import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

import ican.ytx.com.mediacodectest.media.player.pragma.YtxLog;

/**
 * Created by Administrator on 2016/9/10.
 */

public class GraphicGLSurfaceView extends GLSurfaceView {

    public static final String TAG = "GraphicGLSurfaceView";
    public GraphicRenderer renderer;

    int mWidth;
    int mHeight;

    Picture firstPicture;
    Interpolator mInterpolator = new AccelerateDecelerateInterpolator();

    volatile boolean mIsResume = false;

    volatile boolean isInitial = false;
    private ISurfaceCallback mSurfaceCallback;

    public ISurfaceCallback getSurfaceCallback() {
        return mSurfaceCallback;
    }

    public void setSurfaceCallback(ISurfaceCallback mSurfaceCallback) {
        this.mSurfaceCallback = mSurfaceCallback;
    }



    public GraphicGLSurfaceView(Context context) {
        this(context,null);
    }

    public GraphicGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView(context);
    }

    private void initView(Context context){
        if(!supportsOpenGLES2(context)){
            throw new RuntimeException("not support gles 2.0");
        }

        setEGLContextClientVersion(2);
        setEGLContextFactory(new ContextFactory());
        setEGLConfigChooser(new CustomChooseConfig.ComponentSizeChooser(8, 8, 8, 8, 0, 0));
        getHolder().setFormat(PixelFormat.RGBA_8888);
        getHolder().addCallback(this);
        renderer = new GraphicRenderer();
        setRenderer(renderer);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);


    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        YtxLog.d(TAG,"#### #### onMeasure getHeight=" + getHeight() +" getWidth="+getWidth());
    }


    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        YtxLog.d(TAG,"#### #### onLayout getHeight=" + getHeight() +" getWidth="+getWidth());

    }

    private boolean supportsOpenGLES2(final Context context) {
        final ActivityManager activityManager = (ActivityManager)
                context.getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo =
                activityManager.getDeviceConfigurationInfo();
        return configurationInfo.reqGlEsVersion >= 0x20000;
    }


    public GraphicRenderer getRenderer(){

        return renderer;
    }

    public interface OnScreenWindowChangedListener {
        void onScreenWindowChanged(boolean isFinger, int width, int height, int x1, int y1, int x2, int y2);
    }

    private OnScreenWindowChangedListener onScreenWindowChangedListener = null;

    public void setOnScreenWindowChangedListener(OnScreenWindowChangedListener listener){
        onScreenWindowChangedListener = listener;
    }

    public void queue(Runnable r) {
        requestRender();
    }

    @Override
    public void onResume() {
        super.onResume();
        mIsResume = true;
        YtxLog.d(TAG,"onResume  isInitial="+isInitial);
    }


    @Override
    public void onPause() {
        super.onPause();
        mIsResume = false;
    }


    protected void initial() {
        YtxLog.d(TAG, "initial");
    }

    protected void release() {
        YtxLog.d(TAG, "release");
        if (firstPicture != null) {
            firstPicture.clear();
        }
    }


    public void drawFrame() {

    }

    public void setPicture(Picture picture) {
        mWidth = picture.width();
        mHeight = picture.height();
    }

    public void updateYuv(byte[] ydata, byte[] udata, byte[] vdata){
        requestRender();
    }


    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        super.surfaceCreated(holder);
        YtxLog.d(TAG,"#### #### surfaceCreated getHeight=" + getHeight() +" getWidth="+getWidth());
        if(mSurfaceCallback != null){
            mSurfaceCallback.onSurfaceCreated(holder);
        }
    }


    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        super.surfaceChanged(holder, format, w, h);
        YtxLog.d(TAG,"#### #### surfaceChanged getHeight=" + getHeight() +" getWidth="+getWidth());
        if(mSurfaceCallback != null){
            mSurfaceCallback.onSurfaceChanged(holder,  format,  w,  h);
        }
    }


    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        super.surfaceDestroyed(holder);
        YtxLog.d(TAG,"#### #### surfaceDestroyed getHeight=" + getHeight() +" getWidth="+getWidth());
        if(mSurfaceCallback != null){
            mSurfaceCallback.onSurfaceDestroyed(holder);
        }
    }

    public interface ISurfaceCallback {
        /**
         * @param holder

         */
        void onSurfaceCreated(@NonNull SurfaceHolder holder);

        /**
         * @param holder
         * @param format could be 0
         * @param width
         * @param height
         */
        void onSurfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height);

        void onSurfaceDestroyed(@NonNull SurfaceHolder holder);
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            Log.w(TAG, "creating OpenGL ES 2.0 context");
            checkEglError("Before eglCreateContext", egl);
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
            EGLContext context = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
            checkEglError("After eglCreateContext", egl);
            return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }

}
