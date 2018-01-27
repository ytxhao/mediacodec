package ican.ytx.com.mediacodectest.media.player.permission;

import java.util.List;

/**
 * Created by Administrator on 2017/9/6.
 */

public interface PermissionRequestListener {
    /**
     * granted all permissions
     */
    void onPermissionGranted(int requestCode);

    void onPermissionsDenied(int requestCode, List<String> deniedList);

}