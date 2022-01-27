#pragma once

#include "math_types.h"
#include "logging.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Implements a templated vector with 3 coordinates.
	 *
	 * @tparam T The type of the vector coordinates
	 */
	template<typename T>
	struct Vec2
	{
		/* Vec2 static values. */
		/// @{
		static const Vec2 zero;
		static const Vec2 one;
		static const Vec2 right;
		static const Vec2 up;
		static const Vec2 left;
		static const Vec2 down;
		/// @}

		union
		{
			/* Vector space coordinates. */
			struct { T x, y; };

			/* Vector coordinates array. */
			T coords[2];
		};


		/**
		 * @brief Constructs a zero-initialized Vec2.
		 */
		constexpr FORCE_INLINE Vec2() : coords{} {}

		/**
		 * @brief Constructs a new Vec2 and set all coordinates equal to the
		 * given scalar value.
		 *
		 * @param s A scalar value
		 */
		constexpr FORCE_INLINE Vec2(T s) : coords{s, s} {}

		/**
		 * @brief Constructs a new Vec2 with the given coordinates.
		 *
		 * @param x,y The value of the coordinates
		 */
		constexpr FORCE_INLINE Vec2(T x, T y) : coords{x, y} {}

		/**
		 * @brief Returns a ref to the coordinate at i-th position.
		 *
		 * If the position is out of range, the result is undefined.
		 *
		 * @param idx Position of the coordinate.
		 * @return Ref to i-th coordinate
		 * @{
		 */
		constexpr FORCE_INLINE T& operator[](int idx)
		{
			VW_CHECK(idx >= 0 && idx < 2);
			return coords[idx];
		}

		constexpr FORCE_INLINE T const& operator[](int idx) const
		{
			return const_cast<Vec2&>(*this)[idx];
		}
		/// @}

		/**
		 * @brief Returns the squared size of this vector.
		 */
		constexpr FORCE_INLINE T getSize2() const
		{
			return dot(*this);
		}

		/**
		 * @brief Returns the size of a floating-point vector.
		 */
		constexpr FORCE_INLINE T getSize() const
		{
			// Only defined for floating-point coordiante types
			return {};
		}

		/**
		 * @brief Divides all coordinates by the vector's size.
		 *
		 * If the vector is not a floating-point vector, the result is
		 * undefined.
		 */
		constexpr FORCE_INLINE Vec2& normalize()
		{
			return *this /= getSize();
		}

		/**
		 * @brief Returns a new Vec2 equal to this vector divided by its size.
		 *
		 * If the vector is not a floating-point vector, the result is
		 * undefined.
		 */
		constexpr FORCE_INLINE Vec2 getNormal() const
		{
			return *this / getSize();
		}

		/**
		 * @brief Returns a new Vec2 with same size but opposite direction.
		 */
		constexpr FORCE_INLINE Vec2 operator-() const
		{
			return {-x, -y};
		}

		/**
		 * @brief Sums another vector to this vector, coordinate-wise.
		 *
		 * @param other Another Vec2
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec2& operator+=(Vec2 const& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		/**
		 * @brief Subtracts another vector from this vector, coordinate-wise.
		 *
		 * @param other Another Vec2
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec2& operator-=(Vec2 const& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		/**
		 * @brief Multiply this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec2
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec2& operator*=(Vec2 const& other)
		{
			x *= other.x;
			y *= other.y;
			return *this;
		}

		/**
		 * @brief Divide this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec2
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec2& operator/=(Vec2 const& other)
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}

		/**
		 * @brief Returns the coordinate-wise sum of two vectors.
		 *
		 * @param other Another Vec2
		 * @return u + v
		 */
		constexpr FORCE_INLINE Vec2 operator+(Vec2 const& other) const
		{
			return Vec2{*this} += other;
		}

		/**
		 * @brief Returns the coordinate-wise difference of two vectors.
		 *
		 * @param other Another Vec2
		 * @return u - v
		 */
		constexpr FORCE_INLINE Vec2 operator-(Vec2 const& other) const
		{
			return Vec2{*this} -= other;
		}

		/**
		 * @brief Returns the coordinate-wise product of two vectors.
		 *
		 * @param other Another Vec2
		 * @return u * v
		 */
		constexpr FORCE_INLINE Vec2 operator*(Vec2 const& other) const
		{
			return Vec2{*this} *= other;
		}

		/**
		 * @brief Returns the coordinate-wise division of two vectors.
		 *
		 * @param other Another Vec2
		 * @return u / v
		 */
		constexpr FORCE_INLINE Vec2 operator/(Vec2 const& other) const
		{
			return Vec2{*this} /= other;
		}

		/**
		 * @brief Returns the sum of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec2
		 * @return <s, s> + v
		 */
		friend constexpr FORCE_INLINE Vec2 operator+(T const& s, Vec2 const& v)
		{
			return v + s;
		}

		/**
		 * @brief Returns the difference of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec2
		 * @return <s, s> - v
		 */
		friend constexpr FORCE_INLINE Vec2 operator-(T const& s, Vec2 const& v)
		{
			return {s - v.x, s - v.y};
		}

		/**
		 * @brief Returns the product of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec2
		 * @return <s, s> * v
		 */
		friend constexpr FORCE_INLINE Vec2 operator*(T const& s, Vec2 const& v)
		{
			return v * s;
		}

		/**
		 * @brief Returns the division of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec2
		 * @return <s, s> / v
		 */
		friend constexpr FORCE_INLINE Vec2 operator/(T const& s, Vec2 const& v)
		{
			return {s / v.x, s / v.y};
		}

		/**
		 * @brief Returns the dot product of two vectors.
		 *
		 * @param other Another Vec2
		 * @return u dot v
		 */
		constexpr FORCE_INLINE T dot(Vec2 const& other) const
		{
			return x * other.x + y * other.y;
		}

		/**
		 * @brief Returns the cross product of two vectors.
		 *
		 * The return value is the Z coordinate of the corresponding Vec3.
		 *
		 * @param other Another Vec2
		 * @return (u cross v).z
		 */
		constexpr FORCE_INLINE T cross(Vec2 const& other) const
		{
			return x * other.y - y * other.x;
		}

		/**
		 * @brief Converts this Vec2 to a Vec2 with different coordinates type.
		 */
		template<typename U>
		constexpr FORCE_INLINE explicit operator Vec2<U>() const
		{
			return {static_cast<U>(x), static_cast<U>(y)};
		}
	};


	// ==================
	// Vec2 static values
	// ==================
	template<typename T>
	constexpr Vec2<T> Vec2<T>::zero = {};

	template<typename T>
	constexpr Vec2<T> Vec2<T>::one = {1, 1};

	template<typename T>
	constexpr Vec2<T> Vec2<T>::right = {1, 0};

	template<typename T>
	constexpr Vec2<T> Vec2<T>::up = {0, 1};

	template<typename T>
	constexpr Vec2<T> Vec2<T>::left = {-1, 0};

	template<typename T>
	constexpr Vec2<T> Vec2<T>::down = {0, -1};
} // namespace VaporWorldVR::Math
