LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= hookJni.c

LOCAL_LDLIBS    := -llog

LOCAL_MODULE:= hookJni

include $(BUILD_SHARED_LIBRARY)
