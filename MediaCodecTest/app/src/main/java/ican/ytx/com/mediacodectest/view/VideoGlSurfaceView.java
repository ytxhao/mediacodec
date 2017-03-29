package ican.ytx.com.mediacodectest.view;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.concurrent.LinkedBlockingQueue;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

import ican.ytx.com.mediacodectest.AndroidHardwareCodecUtils;


/**
 * Created by Administrator on 2017/3/24.
 */

public class VideoGlSurfaceView extends GLSurfaceView {


    private static final String TAG = "VideoGlSurfaceView";


    volatile boolean updateSurface = false;
    volatile boolean mIsResume = false;
    volatile boolean isInitial = false;

    private GraphicRenderer renderer;
    private VideoDecodeThread mVideoDecodeThread;
    private VideoRefreshThread mVideoRefreshThread;
    private Image mImage;
    private Image mTextureImage;

    private Filter mFilter;
    private Image mMiddlePhoto;
    private GlslFilter mTextureFilter;

    private int mSurfaceTextureId;
    private float[] mTextureMatrix = new float[16];
    public LinkedBlockingQueue<VideoFrame> mVFrameQueue = new LinkedBlockingQueue<VideoFrame>(30);
    public VideoGlSurfaceView(Context context) {
        super(context);
        initView(context);
    }

    public VideoGlSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView(context);
    }

    private void initView(Context context) {
        if (!supportsOpenGLES2(context)) {
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
        renderer.setGlSurfaceView(this);


    }


    private void initial() {

//        mTextureFilter = new GlslFilter(getContext());
//        mTextureFilter.setType(GlslFilter.GL_TEXTURE_EXTERNAL_OES);
//        mTextureFilter.initial();
//        mSurfaceTextureId = RendererUtils.createTexture();
//
//        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mSurfaceTextureId);
//        RendererUtils.checkGlError("glBindTexture mTextureID");
//        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
//                GLES20.GL_TEXTURE_MIN_FILTER,
//                GLES20.GL_NEAREST);
//        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES,
//                GLES20.GL_TEXTURE_MAG_FILTER,
//                GLES20.GL_LINEAR);
//        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S,
//                GLES20.GL_CLAMP_TO_EDGE);
//        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T,
//                GLES20.GL_CLAMP_TO_EDGE);
//
//
//        RendererUtils.checkGlError("surfaceCreated");
//        updateSurface = false;

        mVideoRefreshThread = new VideoRefreshThread(this);
        mVideoRefreshThread.start();

        mVideoDecodeThread = new VideoDecodeThread(this);
        mVideoDecodeThread.start();

    }


    public void drawFrame() {

        if (mVideoDecodeThread == null) {
            return;
        }

        int videoWith = mVideoDecodeThread.getVideoWidth();
        int videoHeight = mVideoDecodeThread.getVideoHeight();
        if (videoWith == 0 || videoHeight == 0) {
            return;
        }
        long lastTime = System.currentTimeMillis();

        if (mImage == null) {
            mImage = Image.create(videoWith, videoHeight);
        } else {
            mImage.updateSize(videoWith, videoHeight);
        }

        if (mTextureImage == null) {
            mTextureImage = new Image(mSurfaceTextureId, videoWith, videoHeight);
        }

        synchronized (this) {
            if (updateSurface) {
                mVideoDecodeThread.updateSurfaceTexture(mTextureMatrix);
                mTextureFilter.updateTextureMatrix(mTextureMatrix);
                updateSurface = false;
            }

            RendererUtils.checkGlError("drawFrame");
            mTextureFilter.process(mTextureImage, mImage);
            Image dst = appFilter(mImage);
            setImage(dst);
        }
    }


    public void queue(Runnable r) {
        renderer.queue.add(r);
        requestRender();
    }

    public void remove(Runnable runnable) {
        renderer.queue.remove(runnable);
    }

    public void flush() {
        renderer.queue.clear();
    }


    int getSurfaceTextureId() {
        return mSurfaceTextureId;
    }


    synchronized public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        updateSurface = true;
        requestRender();
    }


    protected Image appFilter(Image src) {
        if (mFilter != null) {
            if (mMiddlePhoto == null && src != null) {
                mMiddlePhoto = Image.create(src.width(), src.height());
            }
            mFilter.process(src, mMiddlePhoto);
            return mMiddlePhoto;
        }
        return src;
    }

    public void setImage(Image image) {
        renderer.setImage(image);
    }

    public void setVideoFrame(VideoFrame mVideoFrame){
        renderer.setVideoFrame(mVideoFrame);
    }

    private boolean supportsOpenGLES2(final Context context) {
        final ActivityManager activityManager = (ActivityManager)
                context.getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo =
                activityManager.getDeviceConfigurationInfo();
        return configurationInfo.reqGlEsVersion >= 0x20000;
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }


    public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        mIsResume = true;
        flush();
        queue(new Runnable() {
            @Override
            public void run() {
                if (!isInitial) {
                    isInitial = true;
                    initial();
                }
            }
        });
    }


    public void onPuase() {
    }

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            Log.w(TAG, "creating OpenGL ES 2.0 context");
            checkEglError("Before eglCreateContext", egl);
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE};
            EGLContext context = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
            checkEglError("After eglCreateContext", egl);
            return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }

    static class VideoRefreshThread extends WorkThread{

        private WeakReference<VideoGlSurfaceView> mVideoGlSurfaceViewGPURef;
        private long time;
        private long delay;
        private long frameTimer;
        private long remainingTimeMillis;
        private int remainingTimeNanos;
        private double AV_SYNC_THRESHOLD_MAX = 0.1;
        private boolean isRefresh = false;
        private VideoFrame mFrameCurrent;
        private VideoFrame mFrameNext;
        public VideoRefreshThread(VideoGlSurfaceView mVideoGlSurfaceViewGPURef) {
            super("VideoRefreshThread");
            this.mVideoGlSurfaceViewGPURef = new WeakReference<VideoGlSurfaceView>(mVideoGlSurfaceViewGPURef);

        }

        @Override
        protected int doRepeatWork() throws InterruptedException {

            if(remainingTimeNanos > 0){
                sleep(0, remainingTimeNanos);
            }
            remainingTimeNanos = 0;

            if (mVideoGlSurfaceViewGPURef != null && mVideoGlSurfaceViewGPURef.get() != null) {

//                if(!isRefresh){
//                    isRefresh = true;
//
//                    if(mFrameNext != null){
//                        mFrameCurrent = mFrameNext;
//                    }else{
                        mFrameCurrent = mVideoGlSurfaceViewGPURef.get().mVFrameQueue.take();
//                    }
//                    mFrameNext = mVideoGlSurfaceViewGPURef.get().mVFrameQueue.take();
//
//                }
//
//                time = System.currentTimeMillis();
//                if(mFrameNext.timeStamp >= mFrameCurrent.timeStamp){
//                    delay = mFrameNext.timeStamp - mFrameCurrent.timeStamp;
//                }
//
//                if (time < frameTimer + delay) { //如果当前时间小于(frame_timer+delay)则不去frameQueue取下一帧直接刷新当前帧
//                    remainingTimeMillis = Math.min(frameTimer + delay - time, remainingTimeMillis); //显示下一帧还差多长时间
//                    return 0;
//                }
//
//
//
//                frameTimer += delay; //下一帧需要在这个时间显示
//                if (delay > 0 && time - frameTimer > AV_SYNC_THRESHOLD_MAX) {
//                    frameTimer = time;
//                }

                mVideoGlSurfaceViewGPURef.get().setVideoFrame(mFrameCurrent);
                mVideoGlSurfaceViewGPURef.get().requestRender();
                isRefresh = false;

            }

            return 0;
        }

        @Override
        protected void doInitial() {

        }

        @Override
        protected void doRelease() {

        }
    }

    static class VideoDecodeThread extends WorkThread {

        private static final String[] filePath = new String[]{
                "/storage/emulated/0/titanic.mkv",
                "/storage/emulated/0/video2.mp4",
                "/storage/emulated/0/gqfc07.ts",
                "/storage/emulated/0/test_file/x7_11.mkv",
        };

        private static final int COLOR_FormatI420 = 1;
        private static final int COLOR_FormatNV21 = 2;
        private static final int DEQUEUE_INPUT_TIMEOUT = 2000;
        private static final int DEQUEUE_OUTPUT_TIMEOUT = 2000;

        private volatile boolean mInitialError = false;

        private int mVideoWidth;
        private int mVideoHeight;

        private MediaFormat mediaFormat;
        private MediaExtractor extractor = null;
        private MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        private MediaCodec decoder;

        private Surface mSurface;
        private SurfaceTexture mSurfaceTexture;
        FileOutputStream outStream;
        private AndroidHardwareCodecUtils.DecoderProperties decoderProperty;

        private WeakReference<VideoGlSurfaceView> mVideoGlSurfaceViewGPURef;

        public VideoDecodeThread(VideoGlSurfaceView mVideoGlSurfaceViewGPURef) {
            super("VideoDecodeThread");
            Log.d(TAG, "VideoDecodeThread start");
            this.mVideoGlSurfaceViewGPURef = new WeakReference<VideoGlSurfaceView>(mVideoGlSurfaceViewGPURef);
        }


        public int getVideoWidth() {
            return mVideoWidth;
        }

        public int getVideoHeight() {
            return mVideoHeight;
        }

        private void compressToJpeg(String fileName, android.media.Image image) {
            FileOutputStream outStream;
            try {
                outStream = new FileOutputStream(fileName);
            } catch (IOException ioe) {
                throw new RuntimeException("Unable to create output file " + fileName, ioe);
            }
            Rect rect = image.getCropRect();
            YuvImage yuvImage = new YuvImage(getDataFromImage(image, COLOR_FormatNV21), ImageFormat.NV21, rect.width(), rect.height(), null);
            yuvImage.compressToJpeg(rect, 100, outStream);
        }

        private static boolean isImageFormatSupported(android.media.Image image) {
            int format = image.getFormat();
            switch (format) {
                case ImageFormat.YUV_420_888:
                case ImageFormat.NV21:
                case ImageFormat.YV12:
                    return true;
            }
            return false;
        }

        private static byte[] getDataFromImage(android.media.Image image, int colorFormat) {
            if (colorFormat != COLOR_FormatI420 && colorFormat != COLOR_FormatNV21) {
                throw new IllegalArgumentException("only support COLOR_FormatI420 " + "and COLOR_FormatNV21");
            }
            if (!isImageFormatSupported(image)) {
                throw new RuntimeException("can't convert Image to byte array, format " + image.getFormat());
            }
            Rect crop = image.getCropRect();
            int format = image.getFormat();
            int width = crop.width();
            int height = crop.height();
            android.media.Image.Plane[] planes = image.getPlanes();
            byte[] data = new byte[width * height * ImageFormat.getBitsPerPixel(format) / 8];
            byte[] rowData = new byte[planes[0].getRowStride()];
            Log.v(TAG, "get data from " + planes.length + " planes");
            int channelOffset = 0;
            int outputStride = 1;
            for (int i = 0; i < planes.length; i++) {
                switch (i) {
                    case 0:
                        channelOffset = 0;
                        outputStride = 1;
                        break;
                    case 1:
                        if (colorFormat == COLOR_FormatI420) {
                            channelOffset = width * height;
                            outputStride = 1;
                        } else if (colorFormat == COLOR_FormatNV21) {
                            channelOffset = width * height + 1;
                            outputStride = 2;
                        }
                        break;
                    case 2:
                        if (colorFormat == COLOR_FormatI420) {
                            channelOffset = (int) (width * height * 1.25);
                            outputStride = 1;
                        } else if (colorFormat == COLOR_FormatNV21) {
                            channelOffset = width * height;
                            outputStride = 2;
                        }
                        break;
                }
                ByteBuffer buffer = planes[i].getBuffer();
                int rowStride = planes[i].getRowStride();
                int pixelStride = planes[i].getPixelStride();

                Log.v(TAG, "pixelStride " + pixelStride);
                Log.v(TAG, "rowStride " + rowStride);
                Log.v(TAG, "width " + width);
                Log.v(TAG, "height " + height);
                Log.v(TAG, "buffer size " + buffer.remaining());

                int shift = (i == 0) ? 0 : 1;
                int w = width >> shift;
                int h = height >> shift;
                buffer.position(rowStride * (crop.top >> shift) + pixelStride * (crop.left >> shift));
                for (int row = 0; row < h; row++) {
                    int length;
                    if (pixelStride == 1 && outputStride == 1) {
                        length = w;
                        buffer.get(data, channelOffset, length);
                        channelOffset += length;
                    } else {
                        length = (w - 1) * pixelStride + 1;
                        buffer.get(rowData, 0, length);
                        for (int col = 0; col < w; col++) {
                            data[channelOffset] = rowData[col * pixelStride];
                            channelOffset += outputStride;
                        }
                    }
                    if (row < h - 1) {
                        buffer.position(buffer.position() + rowStride - length);
                    }
                }
                Log.v(TAG, "Finished reading data from plane " + i);
            }
            return data;
        }

        @Override
        protected int doRepeatWork() throws InterruptedException {

            if (!mIsRunning)
                return 0;

            long lastTime = System.currentTimeMillis();

            if (mInitialError) {
                return 0;
            }

            if (decoder == null) {
                releaseMediaDecode();
                configureMediaDecode();
            }

            if (decoder == null) {
                return 0;
            }

            int inputBufIndex = decoder.dequeueInputBuffer(DEQUEUE_INPUT_TIMEOUT);
            if (inputBufIndex >= 0) {
                ByteBuffer inputBuffer = decoder.getInputBuffer(inputBufIndex);
                int sampleSize = extractor.readSampleData(inputBuffer, 0);
                if (sampleSize < 0) {
                    decoder.queueInputBuffer(inputBufIndex, 0, 0, 0L, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                } else {
                    long presentationTimeUs = extractor.getSampleTime();
                   // Log.d(TAG,"presentationTimeUs="+presentationTimeUs);
                    decoder.queueInputBuffer(inputBufIndex, 0, sampleSize, presentationTimeUs, 0);
                    extractor.advance();
                }

            }

            while (true) {
                if (!mIsRunning)
                    return 0;
                int res = decoder.dequeueOutputBuffer(info, DEQUEUE_OUTPUT_TIMEOUT);
                if (res >= 0) {
                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        /**
                         * 文件已读取到结尾
                         */
                        //sawOutputEOS = true;
                        try {
                            outStream.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                    boolean doRender = (info.size != 0);

                    if(doRender){

                        MediaFormat outformat = decoder.getOutputFormat();
                        mVideoWidth = outformat.getInteger(MediaFormat.KEY_WIDTH);
                        mVideoHeight = outformat.getInteger(MediaFormat.KEY_HEIGHT);
                        android.media.Image image = decoder.getOutputImage(res);

//                        ByteBuffer buffer = image.getPlanes()[0].getBuffer();
//                        byte[] arr = new byte[buffer.remaining()];
//                        buffer.get(arr);

                        VideoFrame mFrame = new VideoFrame();
                        mFrame.timeStamp = info.presentationTimeUs;
                        mFrame.height = mVideoHeight;
                        mFrame.width = mVideoWidth;
                        mFrame.data = getDataFromImage(image, COLOR_FormatI420);

                        try {
                            outStream.write(mFrame.data);
                        } catch (IOException ioe) {
                            throw new RuntimeException("failed writing data to file ", ioe);
                        }

                        if (mVideoGlSurfaceViewGPURef != null && mVideoGlSurfaceViewGPURef.get() != null) {
                            mVideoGlSurfaceViewGPURef.get().mVFrameQueue.put(mFrame);
                        }

                        decoder.releaseOutputBuffer(res, true);
                    }

                } else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                } else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                } else {

                    break;
                }
            }

            long decodeOneFrameMilliseconds = System.currentTimeMillis() - lastTime;

            //Log.d(TAG, "decode "  + ", decodeTime:" + decodeOneFrameMilliseconds);

            return 0;
        }

        @Override
        protected void doInitial() {
            Log.d(TAG, " doInitial");
            int surfaceId = 0;
            File videoFile = new File(filePath[0]);

            extractor = new MediaExtractor();
            try {
                extractor.setDataSource(videoFile.toString());
            } catch (IOException e) {
                e.printStackTrace();
            }
            int trackIndex = selectTrack(extractor);
            if (trackIndex < 0) {
                throw new RuntimeException("No video track found in " + filePath);
            }

            extractor.selectTrack(trackIndex);
            mediaFormat = extractor.getTrackFormat(trackIndex);
            String mime = mediaFormat.getString(MediaFormat.KEY_MIME);
            decoderProperty = AndroidHardwareCodecUtils.findAVCDecoder(mime);
            if (mVideoGlSurfaceViewGPURef != null && mVideoGlSurfaceViewGPURef.get() != null) {
                surfaceId = mVideoGlSurfaceViewGPURef.get().getSurfaceTextureId();
            }

            mSurfaceTexture = new SurfaceTexture(surfaceId);
            mSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {

                @Override
                public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                    if (mVideoGlSurfaceViewGPURef != null
                            && mVideoGlSurfaceViewGPURef.get() != null) {
                        mVideoGlSurfaceViewGPURef.get().onFrameAvailable(surfaceTexture);
                    }
                }
            });

            mSurface = new Surface(mSurfaceTexture);
            mInitialError = false;

            try {
                outStream = new FileOutputStream("/storage/emulated/0/test.yuv");
            } catch (IOException ioe) {
                throw new RuntimeException("Unable to create output file ", ioe);
            }
        }

        @Override
        protected void doRelease() {
            Log.d(TAG, " doRelease");

            mSurfaceTexture.setOnFrameAvailableListener(null);
            mSurfaceTexture.release();
            mSurface.release();

            releaseMediaDecode();

            Log.d(TAG, " VideoDecodeThread stop");
        }

        void configureMediaDecode() {
            try {

                Log.d(TAG, "Codec Name--------" + decoderProperty.codecName +
                        "Codec Format--------" + decoderProperty.colorFormat);
                try {
                    decoder = MediaCodec.createByCodecName(decoderProperty.codecName);
                } catch (Exception e) {
                    e.printStackTrace();
                }

                mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                        decoderProperty.colorFormat);
               // decoder.configure(mediaFormat, mSurface, null, 0);
                decoder.configure(mediaFormat, null, null, 0);
                decoder.start();
            } catch (Exception e) {
                mInitialError = true;
                releaseMediaDecode();

            }
        }

        void releaseMediaDecode() {
            Log.d(TAG, " releaseMediaDecode");
            if (decoder != null) {
                try {
                    decoder.stop();
                    decoder.release();
                    decoder = null;
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }

        public void updateSurfaceTexture(float[] mtx) {
            mSurfaceTexture.updateTexImage();
            mSurfaceTexture.getTransformMatrix(mtx);
        }


        private static int selectTrack(MediaExtractor extractor) {
            int numTracks = extractor.getTrackCount();
            for (int i = 0; i < numTracks; i++) {
                MediaFormat format = extractor.getTrackFormat(i);
                String mime = format.getString(MediaFormat.KEY_MIME);
                Log.d(TAG, "Extractor selected track " + i + " (" + mime + "): " + format);
                if (mime.startsWith("video/")) {
                    Log.d(TAG, "Extractor selected video track " + i + " (" + mime + "): " + format);
                    return i;
                }
            }
            return -1;
        }
    }

}
