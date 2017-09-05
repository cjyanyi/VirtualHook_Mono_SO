package com.lody.virtual.server.pm;

import com.lody.virtual.helper.collection.ArrayMap;
import com.lody.virtual.helper.utils.VLog;
import com.lody.virtual.server.pm.parser.PackageParserEx;
import com.lody.virtual.server.pm.parser.VPackage;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Created by liuruikai756 on 17/07/2017.
 */

public class HookCacheManager {
    static final Set<String> HOOK_PLUGIN_NAME_CACHE = new HashSet<>();

    public static int size() {
        synchronized (HOOK_PLUGIN_NAME_CACHE) {
            return HOOK_PLUGIN_NAME_CACHE.size();
        }
    }

    public static void put(String packageName) {
        synchronized (HookCacheManager.class) {
            HOOK_PLUGIN_NAME_CACHE.add(packageName);
        }
    }

    public static boolean remove(String packageName) {
        synchronized (HookCacheManager.class) {
            return HOOK_PLUGIN_NAME_CACHE.remove(packageName);
        }
    }

    public static String[] getAll() {
        synchronized (HookCacheManager.class) {
            return HOOK_PLUGIN_NAME_CACHE.toArray(new String[HOOK_PLUGIN_NAME_CACHE.size()]);
        }
    }
}
