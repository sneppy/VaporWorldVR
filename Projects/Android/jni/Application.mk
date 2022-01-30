# Obtain Application.mk dir, then set root dir
ROOT_DIR := $(dir $(lastword $(MAKEFILE_LIST)))../../../../../

# Set the module path equal to the root dir
NDK_MODULE_PATH := $(ROOT_DIR)

# ndk-r14 introduced failure for missing dependencies. If 'false', the clean
# step will error as we currently remove prebuilt artifacts on clean.
APP_ALLOW_MISSING_DEPS=true

# Enable ASan (Address Sanitizer)
# See https://developer.android.com/ndk/guides/asan
#APP_STL := c++_shared
#APP_CFLAGS := -fsanitize=address -fno-omit-frame-pointer
#APP_LDFLAGS := -fsanitize=address
