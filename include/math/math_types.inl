#ifndef VW_MATH_USE_GCEM
# define VW_MATH_USE_GCEM 1
#endif

#if VW_MATH_USE_GCEM
# include "gcem.hpp"
#else
# include <math.h>
#endif


namespace VaporWorldVR::Math
{
#if VW_MATH_USE_GCEM
	/* Computes the sine of the given value (in radians). */
	/// @{
	constexpr FORCE_INLINE float sin(float x)   { return ::gcem::sin(x); }
	constexpr FORCE_INLINE double sin(double x) { return ::gcem::sin(x); }
	/// @}

	/* Computes the cosine of the given value (in radians). */
	/// @{
	constexpr FORCE_INLINE float cos(float x)   { return ::gcem::cos(x); }
	constexpr FORCE_INLINE double cos(double x) { return ::gcem::cos(x); }
	/// @}

	/* Computes the tangent of the given value (in radians). */
	/// @{
	constexpr FORCE_INLINE float tan(float x)   { return ::gcem::tan(x); }
	constexpr FORCE_INLINE double tan(double x) { return ::gcem::tan(x); }
	/// @}

	/* Computes the arcsine of the given value. */
	/// @{
	constexpr FORCE_INLINE float asin(float x)   { return ::gcem::asin(x); }
	constexpr FORCE_INLINE double asin(double x) { return ::gcem::asin(x); }
	/// @}

	/* Computes the arccosine of the given value. */
	/// @{
	constexpr FORCE_INLINE float acos(float x)   { return ::gcem::acos(x); }
	constexpr FORCE_INLINE double acos(double x) { return ::gcem::acos(x); }
	/// @}

	/* Computes the arctangent of the given value. */
	/// @{
	constexpr FORCE_INLINE float atan(float x)   { return ::gcem::atan(x); }
	constexpr FORCE_INLINE double atan(double x) { return ::gcem::atan(x); }
	/// @}

	/* Computes the arctangent from the sine and cosine of the angle. */
	/// @{
	constexpr FORCE_INLINE float atan(float y, float x)   { return ::gcem::atan2(y, x); }
	constexpr FORCE_INLINE double atan(double y, double x) { return ::gcem::atan2(y, x); }
	/// @}

	/* Computes the square root of the given value. */
	/// @{
	constexpr FORCE_INLINE float sqrt(float x)   { return ::gcem::sqrt(x); }
	constexpr FORCE_INLINE double sqrt(double x) { return ::gcem::sqrt(x); }
	/// @}
#endif

	/**
	 * @brief Returns the inverse square root of the given value.
	 *
	 * The result has very low precision, but it's much faster to compute than
	 * `1.f / sqrt(x)`.
	 *
	 * @param x The value to compute the the inverse of the root of
	 * @return ~ 1 / sqrt(x)
	 */
	constexpr FORCE_INLINE float finvsqrt(float x)
	{
		// https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
		constexpr int magic = 0x5f3759df;
		float h = x * 0.5f;
		union { float f; int n; } cv = {.f = x};
		cv.n = magic - (cv.n >> 1);
		x = cv.f;
		x = x * (1.5f - h * x * x);
		return x;
	}

	/**
	 * @brief Returns a very fast approximation of the square root of the given
	 * value.
	 *
	 * @param x The value to compute the root of
	 * @return ~ sqrt(x)
	 */
	constexpr FORCE_INLINE float fsqrt(float x)
	{
		return 1.f / finvsqrt(x);
	}

	/**
	 * @brief Returns the least between two values.
	 *
	 * @param x The first value
	 * @param y The second value
	 * @return x < y ? x : y
	 */
	constexpr FORCE_INLINE auto min(auto const& x, auto const& y)
	{
		return x < y ? x : y;
	}

	/**
	 * @brief Returns the greatest between two values.
	 *
	 * @param x The first value
	 * @param y The second value
	 * @return x > y ? x : y
	 */
	constexpr FORCE_INLINE auto max(auto const& x, auto const& y)
	{
		return x > y ? x : y;
	}


	/**
	 * @brief Compute the linear interpolation between two values.
	 *
	 * @param x The first value
	 * @param y The second value
	 * @param t The interpolation value
	 * @return x + (y - x) * t
	 */
	constexpr FORCE_INLINE auto lerp(auto const& x, auto const& y, float t)
	{
		return x + (y - x) * t;
	}
} // namespace VaporWorldVR::Math
