#pragma once

#include "vec4.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Returns a new vector where each coordinate is the least between
	 * the coordinates of the given vectors.
	 *
	 * @tparam T Type of the first vector's coordinates
	 * @tparam U Type of the second vector's coordinates
	 * @param u The first vector
	 * @param v The second vector
	 * @return {min(u.x, v.x), min(u.y, v.y), min(u.z, v.z)}
	 */
	template<typename T, typename U>
	constexpr FORCE_INLINE Vec3<decltype(T{} + U{})> vmin(Vec3<T> const& u, Vec3<U> const& v)
	{
		return {min(u.x, v.x), min(u.y, v.y), min(u.z, v.z)};
	}


	/**
	 * @brief Like vmin(), but each coordinate is equal to the greatest between
	 * the coordinates of the given vectors.
	 *
	 * @tparam T Type of the first vector's coordinates
	 * @tparam U Type of the second vector's coordinates
	 * @param u The first vector
	 * @param v The second vector
	 * @return {max(u.x, v.x), max(u.y, v.y), max(u.z, v.z)}
	 */
	template<typename T, typename U>
	constexpr FORCE_INLINE Vec3<decltype(T{} + U{})> vmax(Vec3<T> const& u, Vec3<U> const& v)
	{
		return {max(u.x, v.x), max(u.y, v.y), max(u.z, v.z)};
	}
} // namespace VaporWorldVR::Math
