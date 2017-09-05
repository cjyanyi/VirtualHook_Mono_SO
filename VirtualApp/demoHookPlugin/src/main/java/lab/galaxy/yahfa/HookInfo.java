package lab.galaxy.yahfa;

/**
 * Created by liuruikai756 on 31/03/2017.
 */

public class HookInfo {
    static {
        System.loadLibrary("hookJni");
    }
    public static String[] hookItemNames = {
        //"lab.galaxy.demeHookPlugin.Hook_AssetManager_open",
        //"lab.galaxy.demeHookPlugin.Hook_URL_openConnection",
        "lab.galaxy.demeHookPlugin.Hook_File_init"
    };
}
