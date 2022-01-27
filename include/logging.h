#pragma once

#include <android/log.h>

#include "build.h"


// ===============
// Log definitions
// ===============
#define VW_ANDROID_LOG_TAG "VaporWorldVR"

#if VW_BUILD_DEBUG
# define VW_LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, VW_ANDROID_LOG_TAG, fmt, ##__VA_ARGS__)
# define VW_LOG_ERROR(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, VW_ANDROID_LOG_TAG, fmt, ##__VA_ARGS__)
# define VW_LOG_WARN(fmt, ...) __android_log_print(ANDROID_LOG_WARN, VW_ANDROID_LOG_TAG, fmt, ##__VA_ARGS__)
#else
# define VW_LOG_DEBUG(fmt, ...)
# define VW_LOG_ERROR(fmt, ...)
# define VW_LOG_WARN(fmt, ...)
#endif


// ==================
// Assert definitions
// ==================
#if VW_BUILD_DEBUG
# define VW_ASSERTF(cond, fmt, ...) (static_cast<bool>((cond))\
                                     ? void (0)\
                                     : __android_log_assert(#cond, VW_ANDROID_LOG_TAG, fmt, ##__VA_ARGS__));
# define VW_CHECKF(cond, fmt, ...) (static_cast<bool>((cond))\
                                    ? int (0)\
                                    : __android_log_print(ANDROID_LOG_WARN, VW_ANDROID_LOG_TAG, fmt, ##__VA_ARGS__));
# define VW_ASSERT(cond) VW_ASSERTF(cond, #cond) (static_cast<bool>((cond))\
                                                  ? void (0)\
                                                  : __android_log_assert(#cond, VW_ANDROID_LOG_TAG, NULL));
# define VW_CHECK(cond) VW_ASSERTF(cond, #cond) (static_cast<bool>((cond))\
                                                 ? int (0)\
                                                 : __android_log_print(ANDROID_LOG_WARN, VW_ANDROID_LOG_TAG, NULL));
#else
# define VW_ASSERTF(cond, fmt, ...)
# define VW_CHECKF(cond, fmt, ...)
# define VW_ASSERT(cond)
# define VW_CHECK(cond)
#endif
