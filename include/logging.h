#pragma once

#include <unistd.h>

#include <android/log.h>

#include "build.h"


// ===============
// Log definitions
// ===============
#define __VW_ANDROID_LOG_TAG "VaporWorldVR"
#define __VW_LOG_FMT(fmt) "[tid=%d] " fmt, gettid()

#if VW_BUILD_DEBUG
# define VW_LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, __VW_ANDROID_LOG_TAG, __VW_LOG_FMT(fmt),\
                                                    ##__VA_ARGS__)
# define VW_LOG_ERROR(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, __VW_ANDROID_LOG_TAG, __VW_LOG_FMT(fmt),\
                                                    ##__VA_ARGS__)
# define VW_LOG_WARN(fmt, ...) __android_log_print(ANDROID_LOG_WARN, __VW_ANDROID_LOG_TAG, __VW_LOG_FMT(fmt),\
                                                   ##__VA_ARGS__)
#else
# define VW_LOG_DEBUG(fmt, ...)
# define VW_LOG_ERROR(fmt, ...)
# define VW_LOG_WARN(fmt, ...)
#endif


// ==================
// Assert definitions
// ==================
#define __VW_ASSERT_FMT(fmt) "%s:%d: [tid=%d] " fmt, __FILE__, __LINE__, gettid()

#if VW_BUILD_DEBUG
# define VW_ASSERTF(cond, fmt, ...) (static_cast<bool>((cond))\
                                     ? void (0)\
                                     : __android_log_assert(#cond, __VW_ANDROID_LOG_TAG, __VW_ASSERT_FMT(fmt),\
                                                            ##__VA_ARGS__));
# define VW_CHECKF(cond, fmt, ...) (static_cast<bool>((cond))\
                                    ? int (0)\
                                    : __android_log_print(ANDROID_LOG_WARN, __VW_ANDROID_LOG_TAG, __VW_ASSERT_FMT(fmt),\
                                                          ##__VA_ARGS__));
# define VW_ASSERT(cond) VW_ASSERTF(cond, #cond)
# define VW_CHECK(cond) VW_ASSERTF(cond, #cond)
#else
# define VW_ASSERTF(cond, fmt, ...)
# define VW_CHECKF(cond, fmt, ...)
# define VW_ASSERT(cond)
# define VW_CHECK(cond)
#endif
