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
# define VW_LOG(verb, fmt, ...) __android_log_print(verb, __VW_ANDROID_LOG_TAG, __VW_LOG_FMT(fmt), ##__VA_ARGS__)
# define VW_LOG_IF(cond, verb, fmt, ...) (static_cast<bool>((cond))\
                                          ? __android_log_print(verb, __VW_ANDROID_LOG_TAG, __VW_LOG_FMT(fmt),\
                                                                ##__VA_ARGS__)\
                                          : int(0))
#else
# define VW_LOG(verb, tag, fmt, ...)
# define VW_LOG_IF(cond, verb, tag, fmt, ...)
#endif

# define VW_LOG_DEBUG(fmt, ...) VW_LOG(ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
# define VW_LOG_ERROR(fmt, ...) VW_LOG(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
# define VW_LOG_WARN(fmt, ...) VW_LOG(ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)

#define VW_LOG_DEBUG_IF(cond, fmt, ...) VW_LOG_IF(cond, ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
#define VW_LOG_ERROR_IF(cond, fmt, ...) VW_LOG_IF(cond, ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
#define VW_LOG_WARN_IF(cond, fmt, ...) VW_LOG_IF(cond, ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)


// ==================
// Assert definitions
// ==================
#define __VW_ASSERT_FMT(fmt) "%s:%d: [tid=%d] " fmt, __FILE__, __LINE__, gettid()

#if VW_BUILD_DEBUG
# define VW_ASSERTF(cond, fmt, ...) (static_cast<bool>((cond))\
                                     ? void(0)\
                                     : __android_log_assert(#cond, __VW_ANDROID_LOG_TAG, __VW_ASSERT_FMT(fmt),\
                                                            ##__VA_ARGS__));
# define VW_CHECKF(cond, fmt, ...) (static_cast<bool>((cond))\
                                    ? int(0)\
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
