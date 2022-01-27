LOCAL_PATH := $(call my-dir)

# Cleans the LOCAL_* variables
include $(CLEAR_VARS)

# Define shared library source files and compilation flags
LOCAL_MODULE := vaporworldvr
LOCAL_SRC_FILES := ../../../src/vaporworldvr.cpp\
                   ../../../src/runnable_thread.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include
LOCAL_CFLAGS += -std=c11 -Werror
LOCAL_CPPFLAGS += -std=c++2a -Werror
LOCAL_LDLIBS := -lEGL -lGLESv3 -landroid -llog
LOCAL_SHARED_LIBRARIES := vrapi

# This script builds the shared library
include $(BUILD_SHARED_LIBRARY)

$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
