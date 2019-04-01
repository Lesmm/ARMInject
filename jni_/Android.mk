LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_MODULE_TAGS=eng

LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog 

LOCAL_SRC_FILES := hello.cpp JniHelper.cpp

LOCAL_ARM_MODE := arm

LOCAL_MODULE:= hello

LOCAL_CFLAGS    := -I./jni/include/ -I./jni/dalvik/vm/ -I./jni/dalvik -DHAVE_LITTLE_ENDIAN

LOCAL_LDFLAGS	:=	-L./jni/lib/  -L$(SYSROOT)/usr/lib -llog -ldvm -landroid_runtime  -lart

LOCAL_SYSTEM_SHARED_LIBRARIES := libc libdl

include $(BUILD_SHARED_LIBRARY)
