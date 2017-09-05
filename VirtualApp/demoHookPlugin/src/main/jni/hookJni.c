#include "jni.h"
#include <android/log.h>
#include <dlfcn.h>
#include <stddef.h>
#include <string.h>

#define LOG_TAG "YAHFA"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


typedef void (*MSHookType)(void *symbol, void *replace, void **result);
//typedef void *(*propertyFindType)(const char *name);
//typedef int (*propertyReadType)(const void *pi, char *name, char *value);

//static propertyFindType propertyFind;
//static propertyReadType propertyRead;

typedef struct {
    /*
     * The number of assemblies which reference this MonoImage though their 'image'
     * field plus the number of images which reference this MonoImage through their
     * 'modules' field, plus the number of threads holding temporary references to
     * this image between calls of mono_image_open () and mono_image_close ().
     */
    int   ref_count;
    void *raw_data_handle;
    char *raw_data;
    int raw_data_len;
    unsigned char raw_buffer_used :1;
    unsigned char raw_data_allocated :1;
} _MonoImage;

static void *findSymbol(const char *path, const char *symbol) {
    void *handle = NULL;
    if(!strcmp(path,"libmono.so")) {
        handle = dlopen(path, RTLD_LAZY);
        //if(!handle)handle = dlopen("/data/data/com.Company.hello/lib/libmono.so", RTLD_NOW);
    }

    else{
        handle = dlopen(path, RTLD_LAZY);
    }

    if(!handle) {
        LOGE("handle %s is null", path);
        LOGE("Open Error: %s.\n",dlerror());
        return NULL;
    }

    //Cydia::MSHookFunction(void *,void *,void **)
    void *target = dlsym(handle, symbol);
    if(!target) {
        LOGE("symbol %s is null", symbol);
    }
    return target;
}

//************************************************
// Here begin to define hook function
static int* (*old_property_get) (char *data, unsigned int data_len, int need_copy, int *status, int refonly, const char *filename);


static int* new_property_get(char *data, unsigned int data_len, int need_copy, int *status, int refonly, const char *filename) {

    /*for(int i=0; i<propLen; i++) {
        Prop *prop = &props[i];
        if(!strcmp(prop->key, name)) {
            size_t valueLen = strlen(prop->value);
            // Hope there'd be no overflow here!
            strcpy(value, prop->value);
            return valueLen;
        }
    }*/
    LOGI("hook method executed!");
    return old_property_get(data, data_len, need_copy, status, refonly, filename);
}


static void doHook() {
    MSHookType hookFunc;
    // Cydia::MSHookFunction
    hookFunc = findSymbol("libva-native.so", "_ZN5Cydia14MSHookFunctionEPvS0_PS0_");
    //void *target = findSymbol("libc.so", "__system_property_get");
    void *target = findSymbol("libmono.so", "mono_image_open_from_data_with_name");
    if(!hookFunc || !target) {
        LOGE("cannot hook __system_property_get: MSHookFunction %p, __system_property_get %p",
             hookFunc, target);
        return;
    }

    /*
    propertyRead = (propertyReadType)findSymbol("libc.so", "__system_property_read");
    propertyFind = (propertyFindType)findSymbol("libc.so", "__system_property_find");
    if(!propertyRead || !propertyFind) {
        LOGE("cannot find propertyFind and propertyRead: %p, %p", propertyRead, propertyFind);
        return;
    }
     */

    hookFunc(target, (void *)&new_property_get, (void **)&old_property_get);
}




jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    doHook();
    LOGI("native hook done");
    return JNI_VERSION_1_6;
}