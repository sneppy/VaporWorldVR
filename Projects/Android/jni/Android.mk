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
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_EXPORT_LDLIBS := $(LOCAL_LD_LIBS)

# Required to enable ASan (Address Sanitizer)
# See https://developer.android.com/ndk/guides/asan
#LOCAL_ARM_MODE := arm

# This script builds the shared library
include $(BUILD_SHARED_LIBRARY)

# Clear local variables
include $(CLEAR_VARS)

# Define the test module
LOCAL_MODULE := vaporworldvr_test
LOCAL_SRC_FILES := ../../../test/test_math.cpp
LOCAL_CPP_FEATURES := rtti
LOCAL_CFLAGS := -std=c11
LOCAL_CPPFLAGS := -std=c++2a
LOCAL_SHARED_LIBRARIES := vaporworldvr
LOCAL_STATIC_LIBRARIES := googletest_main

# Build the test executable
include $(BUILD_EXECUTABLE)

# Import the VrApi library
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)

# Import gtest (use the built-in NDK gtest)
include $(ANDROID_NDK_HOME)/sources/third_party/googletest/Android.mk
