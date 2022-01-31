LOCAL_PATH := $(call my-dir)

# Cleans the LOCAL_* variables
include $(CLEAR_VARS)

# Define shared library source files and compilation flags
LOCAL_MODULE := vaporworldvr
LOCAL_SRC_FILES := ../../../src/vaporworldvr.cpp\
                   ../../../src/runnable_thread.cpp\
                   ../../../src/thread_utils.cpp\
                   ../../../src/collision_utils.cpp
LOCAL_CPP_FEATURES := rtti
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include\
                    $(LOCAL_PATH)/../../../external/gcem/include
LOCAL_CFLAGS += -std=c11 -Werror
LOCAL_CPPFLAGS += -std=c++2a -Werror
LOCAL_LDLIBS := -lEGL -lGLESv3 -landroid -llog
LOCAL_SHARED_LIBRARIES := vrapi

# Required to enable ASan (Address Sanitizer)
# See https://developer.android.com/ndk/guides/asan
#LOCAL_ARM_MODE := arm

# This script builds the shared library
include $(BUILD_SHARED_LIBRARY)

$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
