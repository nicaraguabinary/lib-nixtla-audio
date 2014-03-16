LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE   := com.nibsa.nixtla.android.demo.captureeco.native
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/../../../../include/
LOCAL_CFLAGS := -O3 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
###

LOCAL_SRC_FILES := ../../../../src/c/nixtla-audio.c com_nibsa_nixtla_android_demo_captureeco_DemoNativeInterface.c

LOCAL_LDLIBS := -llog -lOpenSLES -landroid

include $(BUILD_SHARED_LIBRARY)


