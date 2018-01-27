package ican.ytx.com.mediacodectest.media.player.permission;

import android.support.annotation.NonNull;

/**
 * Created by Administrator on 2017/9/6.
 */

public final class PermissionCallbackManager {
    private static PermissionCallback mPermissionCallback;

    public static void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (mPermissionCallback != null) {
            mPermissionCallback.onPermissionCallback(requestCode, permissions, grantResults);
            mPermissionCallback = null;
        }
    }

    public static void setPermissionCallback(PermissionCallback permissionCallback) {
        mPermissionCallback = permissionCallback;
    }

    public interface PermissionCallback {
        void onPermissionCallback(int requestCode, String[] permissions, int[] grantResults);
    }
}
