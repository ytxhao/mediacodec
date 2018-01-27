package ican.ytx.com.mediacodectest.media.player.permission;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.PermissionChecker;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2017/9/5.
 */

public class PermissionUtil implements PermissionCallbackManager.PermissionCallback {
    private static final String TAG = "PermissionUtil";

    public static final int PERMISSION_REQUEST_CODE_MAIN_INTERNAL = 101;
    public static final int PERMISSION_REQUEST_CODE_MAIN_INTERNATIONAL = 109;
    public static final int PERMISSION_REQUEST_CODE_SETTING_UPDATE = 102;
    public static final int PERMISSION_REQUEST_CODE_DEVICE_SELECT_SCAN = 103;
    public static final int PERMISSION_REQUEST_CODE_SHARE_SCAN = 104;
    public static final int PERMISSION_REQUEST_CODE_LOGIN_SCAN = 105;
    public static final int PERMISSION_REQUEST_CODE_RESET_CAMERA_SCAN = 106;
    public static final int PERMISSION_REQUEST_CODE_CONNECTION_BARCODE_SCAN = 107;
    public static final int PERMISSION_REQUEST_CODE_RECORD_AUDIO = 108;

    private Object mObject;
    private Activity mActivity;
    private PermissionRequestListener mPermissionRequestListener;

    private PermissionUtil(Activity activity) {
        mActivity = activity;
        PermissionCallbackManager.setPermissionCallback(this);
    }

    public static PermissionUtil newInstance(Activity activity) {
        return new PermissionUtil(activity);
    }

    public boolean hasPermission(@NonNull String... permissions) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return true;
        }

        if (permissions.length == 0) {
            return true;
        }

        for (String per : permissions) {
            int result = PermissionChecker.checkSelfPermission(mActivity, per);
            if (result != PermissionChecker.PERMISSION_GRANTED) {
                return false;
            }
        }

        return true;
    }

    public static boolean hasPermission(Context context, @NonNull String... permissions) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return true;
        }

        if (permissions.length == 0) {
            return true;
        }

        for (String per : permissions) {
            int result = PermissionChecker.checkSelfPermission(context, per);
            if (result != PermissionChecker.PERMISSION_GRANTED) {
                return false;
            }
        }

        return true;
    }

    @TargetApi(Build.VERSION_CODES.M)
    public void requestPermission(int requestCode, PermissionRequestListener permissionRequestListener, @NonNull String... permissions) {
        requestPermission(null, requestCode, permissionRequestListener, permissions);
    }

    @TargetApi(Build.VERSION_CODES.M)
    public void requestPermission(Object object, int requestCode, PermissionRequestListener permissionRequestListener, @NonNull String... permissions) {
        mObject = object;
        mPermissionRequestListener = permissionRequestListener;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            if (permissionRequestListener != null) {
                permissionRequestListener.onPermissionGranted(requestCode);
            }
            return;
        }

        List<String> deniedList = new ArrayList<>();
        for (String per : permissions) {
            if (!hasPermission(per)) {
                deniedList.add(per);
            }
        }
        if (deniedList.isEmpty()) {
            if (permissionRequestListener != null) {
                permissionRequestListener.onPermissionGranted(requestCode);
            }
            return;
        }
        ActivityCompat.requestPermissions(mActivity, deniedList.toArray(new String[]{}), requestCode);
    }

    /**
     * 是否不再提醒
     *
     * @param activity
     * @param deniedPermission
     * @return
     */
    public static boolean permissionPermanentlyDenied(Activity activity, String deniedPermission) {
        return !ActivityCompat.shouldShowRequestPermissionRationale(activity, deniedPermission);
    }

    @Override
    public void onPermissionCallback(int requestCode, String[] permissions, int[] grantResults) {
        ArrayList<String> granted = new ArrayList<>(permissions.length);
        ArrayList<String> denied = new ArrayList<>(permissions.length);
        for (int i = 0; i < permissions.length; i++) {
            String perm = permissions[i];
            if (grantResults[i] == PackageManager.PERMISSION_GRANTED && hasPermission(perm)) {
                granted.add(perm);
            } else {
                denied.add(perm);
            }
        }

        if (!granted.isEmpty() && denied.isEmpty() && mObject != null) {
            runAnnotatedMethods(mObject, requestCode);
        }

        if (mPermissionRequestListener != null) {
            if (!granted.isEmpty() && denied.isEmpty()) {
                mPermissionRequestListener.onPermissionGranted(requestCode);
            }
            if (!denied.isEmpty()) {
                mPermissionRequestListener.onPermissionsDenied(requestCode, denied);
            }
        }
    }

    private static void runAnnotatedMethods(Object object, int requestCode) {
        Class clazz = object.getClass();
        if (isUsingAndroidAnnotations(object)) {
            clazz = clazz.getSuperclass();
        }
        for (Method method : clazz.getDeclaredMethods()) {
            if (method.isAnnotationPresent(AfterPermissionGranted.class)) {
                // Check for annotated methods with matching request code.
                AfterPermissionGranted ann = method.getAnnotation(AfterPermissionGranted.class);
                if (ann.value() == requestCode) {
                    // Method must be void so that we can invoke it
                    if (method.getParameterTypes().length > 0) {
                        throw new RuntimeException(
                                "Cannot execute non-void method " + method.getName());
                    }

                    try {
                        // Make method accessible if private
                        if (!method.isAccessible()) {
                            method.setAccessible(true);
                        }
                        method.invoke(object);
                    } catch (IllegalAccessException e) {
                        Log.e(TAG, "runDefaultMethod:IllegalAccessException", e);
                    } catch (InvocationTargetException e) {
                        Log.e(TAG, "runDefaultMethod:InvocationTargetException", e);
                    }
                }
            }
        }
    }

    private static boolean isUsingAndroidAnnotations(Object object) {
        if (!object.getClass().getSimpleName().endsWith("_")) {
            return false;
        }

        try {
            Class clazz = Class.forName("org.androidannotations.api.view.HasViews");
            return clazz.isInstance(object);
        } catch (ClassNotFoundException e) {
            return false;
        }
    }
}
