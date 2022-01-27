#pragma once


// ============
// Build macros
// ============
#ifdef NDEBUG
# define VW_BUILD_DEBUG 0
# define VW_BUILD_RELEASE 1
#else
# define VW_BUILD_DEBUG 1
# define VW_BUILD_RELEASE 0
#endif
