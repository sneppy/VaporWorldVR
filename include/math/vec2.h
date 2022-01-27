#pragma once

#include "math_types.h"
#include "logging.h"


namespace VaporWorldVR::Math
{
	template<typename T>
	struct Vec2
	{
		union
		{
			/* Vector space coordinates. */
			struct { T x, y; };

			/* Vector coordinates array. */
			T coords[2];
		};

		constexpr FORCE_INLINE Vec2() : coords{} {}
	};
} // namespace VaporWorldVR::Math
