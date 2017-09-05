package com.lody.virtual.client;

import android.hardware.Camera;
import android.media.AudioRecord;
import android.os.Binder;
import android.os.Build;
import android.os.Process;
import android.util.Log;

import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.client.env.VirtualRuntime;
import com.lody.virtual.client.ipc.VActivityManager;
import com.lody.virtual.helper.utils.VLog;
import com.lody.virtual.os.VUserHandle;
import com.lody.virtual.remote.InstalledAppInfo;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import dalvik.system.DexFile;

/**
 * VirtualApp Native Project
 */
public class NativeEngine {

    private static final String TAG = NativeEngine.class.getSimpleName();

    private static Map<String, InstalledAppInfo> sDexOverrideMap;
    private static Method gOpenDexFileNative;
    private static Method gCameraNativeSetup;
    private static int gCameraMethodType = -1;
    private static Method gAudioRecordNativeCheckPermission;

    private static boolean sFlag = false;

    static {
        try {
            System.loadLibrary("va-native");
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
    }

    static {
        String methodName =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT ? "openDexFileNative" : "openDexFile";
        for (Method method : DexFile.class.getDeclaredMethods()) {
            if (method.getName().equals(methodName)) {
                gOpenDexFileNative = method;
                break;
            }
        }
        if (gOpenDexFileNative == null) {
            throw new RuntimeException("Unable to find method : " + methodName);
        }
        gOpenDexFileNative.setAccessible(true);


        for(Method method : Camera.class.getDeclaredMethods()) {
            if ("native_setup".equals(method.getName())) {
                gCameraNativeSetup = method;
                Class<?>[] params = method.getParameterTypes();
                switch (params.length) {
                    case 3:
                        gCameraMethodType = 1;
                        break;
                    case 4:
                        if(params[3] == String.class) {
                            gCameraMethodType = 2;
                        }
                        else {
                            gCameraMethodType = 4;
                        }
                        break;
                    case 5:
                        gCameraMethodType = 3;
                        break;
                    default:
                        VLog.w(TAG, "invalid params for native_setup: "+params);
                }
                break;
            }
        }
        if (gCameraNativeSetup != null) {
            gCameraNativeSetup.setAccessible(true);
        }
        else {
            VLog.w(TAG, "gCameraNativeSetup is null");
        }

        for (Method mth : AudioRecord.class.getDeclaredMethods()) {
            if (mth.getName().equals("native_check_permission") && mth.getParameterTypes().length == 1 && mth.getParameterTypes()[0] == String.class) {
                gAudioRecordNativeCheckPermission = mth;
                break;
            }
        }
        if(gAudioRecordNativeCheckPermission != null) {
            gAudioRecordNativeCheckPermission.setAccessible(true);
        }
        else {
            VLog.w(TAG, "gAudioRecordNativeCheckPermission is null");
        }
    }

    public static void startDexOverride() {
        List<InstalledAppInfo> installedAppInfos = VirtualCore.get().getInstalledApps(0);
        sDexOverrideMap = new HashMap<>(installedAppInfos.size());
        for (InstalledAppInfo info : installedAppInfos) {
            try {
                sDexOverrideMap.put(new File(info.apkPath).getCanonicalPath(), info);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static String getRedirectedPath(String origPath) {
        try {
            return nativeGetRedirectedPath(origPath);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
        return origPath;
    }

    public static String restoreRedirectedPath(String origPath) {
        try {
            return nativeRestoreRedirectedPath(origPath);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
        return origPath;
    }

    public static void redirectDirectory(String origPath, String newPath) {
        if (!origPath.endsWith("/")) {
            origPath = origPath + "/";
        }
        if (!newPath.endsWith("/")) {
            newPath = newPath + "/";
        }
        try {
            nativeRedirect(origPath, newPath);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
    }

    public static void redirectFile(String origPath, String newPath) {
        if (origPath.endsWith("/")) {
            origPath = origPath.substring(0, origPath.length() - 1);
        }
        if (newPath.endsWith("/")) {
            newPath = newPath.substring(0, newPath.length() - 1);
        }

        try {
            nativeRedirect(origPath, newPath);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
    }

    public static void readOnly(String path) {
        try {
            nativeReadOnly(path);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
    }

    public static void hook() {
        try {
            nativeStartUniformer();
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
    }

    public static void hookNative() {
        Method[] methods = {gOpenDexFileNative, gCameraNativeSetup, gAudioRecordNativeCheckPermission};
        if (sFlag) {
            return;
        }
        try {
            nativeHookNative(methods, VirtualCore.get().getHostPkg(), VirtualRuntime.isArt(), Build.VERSION.SDK_INT, gCameraMethodType);
        } catch (Throwable e) {
            VLog.e(TAG, VLog.getStackTraceString(e));
        }
        sFlag = true;
    }

    public static void onKillProcess(int pid, int signal) {
        VLog.e(TAG, "killProcess: pid = %d, signal = %d.", pid, signal);
        if (pid == android.os.Process.myPid()) {
            VLog.e(TAG, VLog.getStackTraceString(new Throwable()));
        }
    }

    public static int onGetCallingUid(int originUid) {
        int callingPid = Binder.getCallingPid();
        if (callingPid == Process.myPid()) {
            return VClientImpl.get().getBaseVUid();
        }
        if (callingPid == VirtualCore.get().getSystemPid()) {
            return Process.SYSTEM_UID;
        }
        int vuid = VActivityManager.get().getUidByPid(callingPid);
        if (vuid != -1) {
            return VUserHandle.getAppId(vuid);
        }
        VLog.d(TAG, "Unknown uid: " + callingPid);
        return VClientImpl.get().getBaseVUid();
    }

    public static void onOpenDexFileNative(String[] params) {
        String dexOrJarPath = params[0];
        String outputPath = params[1];
        VLog.d(TAG, "DexOrJarPath = %s, OutputPath = %s.", dexOrJarPath, outputPath);
        try {
            String canonical = new File(dexOrJarPath).getCanonicalPath();
            InstalledAppInfo info = sDexOverrideMap.get(canonical);
            if (info != null && !info.dependSystem) {
                outputPath = info.getOdexFile().getPath();
                params[1] = outputPath;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    private static native void nativeHookNative(Object method, String hostPackageName, boolean isArt, int apiLevel, int cameraMethodType);

    private static native void nativeMark();

    private static native String nativeRestoreRedirectedPath(String redirectedPath);

    private static native String nativeGetRedirectedPath(String orgPath);

    private static native void nativeRedirect(String origPath, String newPath);

    private static native void nativeReadOnly(String path);

    private static native void nativeStartUniformer();

    public static int onGetUid(int uid) {
        return VClientImpl.get().getBaseVUid();
    }

    public static native void nativeHookExec(int apiLevel);
}
