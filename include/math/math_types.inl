#ifndef VW_PLATFORM_USE_GCEM
# define VW_PLATFORM_USE_GCEM 1
#endif

#if VW_PLATFORM_USE_GCEM
# include "gcem.hpp"
#else
# include <math.h>
#endif


namespace VaporWorldVR::Math
{
#if VW_PLATFORM_USE_GCEM
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

	/* Computes the arcsine of the given value. */
	/// @{
	constexpr FORCE_INLINE float acos(float x)   { return ::gcem::acos(x); }
	constexpr FORCE_INLINE double acos(double x) { return ::gcem::acos(x); }
	/// @}

	/* Computes the arcsine of the given value. */
	/// @{
	constexpr FORCE_INLINE float atan(float x)   { return ::gcem::atan(x); }
	constexpr FORCE_INLINE double atan(double x) { return ::gcem::atan(x); }
	/// @}

	/* Computes the square root of the given value. */
	/// @{
	constexpr FORCE_INLINE float sqrt(float x)   { return ::gcem::sqrt(x); }
	constexpr FORCE_INLINE double sqrt(double x) { return ::gcem::sqrt(x); }
	/// @}

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
#endif
} // namespace VaporWorldVR::Math
