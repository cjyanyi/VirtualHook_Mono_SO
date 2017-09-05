package lab.galaxy.demeHookPlugin;

import android.util.Log;

import java.io.File;


/**
 * Created by liuruikai756 on 31/03/2017.
 */

public class Hook_File_init {
    public static String className = "java.io.File";
    public static String methodName = "<init>";
    public static String methodSig = "(Ljava/lang/String;)V";
    public static void hook(File thiz, String fileName) {
        Log.w("YAHFA", "open file "+fileName);
        origin(thiz, fileName);
    }

    public static void origin(File thiz, String fileName) {
        Log.w("YAHFA", "should not be here");
        return;
    }

}
