package ican.ytx.com.mediacodectest.media.player.pragma;

/**
 * Created by Administrator on 2016/9/2.
 */

public interface LibLoader {
    void loadLibrary(String libName) throws UnsatisfiedLinkError,
            SecurityException;
}
