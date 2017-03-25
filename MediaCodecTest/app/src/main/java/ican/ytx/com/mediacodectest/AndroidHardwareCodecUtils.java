package ican.ytx.com.mediacodectest;

import android.annotation.TargetApi;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;
import android.util.Log;

/**
 * Created by Administrator on 2017/3/24.
 */

public class AndroidHardwareCodecUtils {

    public static final String TAG = "AHardwareCodecUtils";
    public static final int COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m = 0x7FA30C04;
    public static final int[] supportedColorList = {
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar,
            MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar,
            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
            MediaCodecInfo.CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
            COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m
    };

    static String blacklisted_decoders[] = {
            // No software decoders
            "omx.google",
            "avcdecoder",
            "avcdecoder_flash",
            "flvdecoder",
            "m2vdecoder",
            "m4vh263decoder",
            "rvdecoder",
            "vc1decoder",
            "vpxdecoder",
            "omx.nvidia.mp4.decode",
            "omx.nvidia.h263.decode",
            "omx.mtk.video.decoder.mpeg4",
            "omx.mtk.video.decoder.h263"
            // End of Rockchip
    };
    static String blacklisted_decoders_Lollipop[] = {
            // No software decoders
            "omx.google",
            "avcdecoder",
            "avcdecoder_flash",
            "flvdecoder",
            "m2vdecoder",
            "m4vh263decoder",
            "rvdecoder",
            "vc1decoder",
            "vpxdecoder",

            // End of Rockchip
    };

    // Helper struct for findVp8Decoder() below.
    public static class DecoderProperties {
        public DecoderProperties(String codecName, int colorFormat) {
            this.codecName = codecName;
            this.colorFormat = colorFormat;
        }

        public final String codecName; // OpenMax component name for VP8 codec.
        public final int colorFormat; // Color format supported by codec.
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    public static DecoderProperties findAVCDecoder(String mime) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN) {
            return null;
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            for (int i = 0; i < MediaCodecList.getCodecCount(); ++i) {
                MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
                if (info.isEncoder()) {
                    continue;
                }

                String codecName = info.getName().toLowerCase();
                Log.d(TAG,"codecName="+codecName);
                if (isInBlack(codecName)) {
                    continue;
                }

                try {
                    // Check if codec supports either yuv420 or nv12.
                    MediaCodecInfo.CodecCapabilities capabilities =
                            info.getCapabilitiesForType(mime);
                    for (int supportedColorFormat : supportedColorList) {
                        for (int codecColorFormat : capabilities.colorFormats) {
                            if (codecColorFormat == supportedColorFormat) {
                                // Found supported HW AVC decoder.
                                Log.d(TAG, "Found target decoder " + info.getName() +
                                        ". Color: 0x" + Integer.toHexString(codecColorFormat));

                                return new DecoderProperties(info.getName(), codecColorFormat);
                            }
                        }
                    }
                } catch (Exception e) {
                    Log.d(TAG, "IllegalArgumentException" + e.toString());
                }
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
            MediaCodecInfo[] codecInfos = list.getCodecInfos();
            for (int i = 0; i < codecInfos.length; ++i) {
                MediaCodecInfo info = codecInfos[i];
                if (info.isEncoder()) {
                    continue;
                }
                String codecName = info.getName().toLowerCase();
                if (isInBlackLollipop(codecName)) {
                    continue;
                }

                try {
                    // Check if codec supports either yuv420 or nv12.
                    MediaCodecInfo.CodecCapabilities capabilities =
                            info.getCapabilitiesForType(mime);
                    for (int codecColorFormat : capabilities.colorFormats) {
                        if (codecColorFormat == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible) {
                            // Found supported HW AVC decoder.
                            Log.d(TAG, "Found target decoder " + info.getName() +
                                    ". Color: 0x" + Integer.toHexString(codecColorFormat));
                            return new DecoderProperties(info.getName(), codecColorFormat);
                        }
                    }
                } catch (Exception e) {
                    Log.d(TAG, "IllegalArgumentException" + e.toString());
                }
            }
        }
        return null;
    }

    private static boolean isInBlack(String codecName) {
        for (String blackCodec : blacklisted_decoders) {
            if (codecName.startsWith(blackCodec)) {
                return true;
            }
        }
        return false;
    }

    private static boolean isInBlackLollipop(String codecName) {
        for (String blackCodec : blacklisted_decoders_Lollipop) {
            if (codecName.startsWith(blackCodec)) {
                return true;
            }
        }
        return false;
    }


}
