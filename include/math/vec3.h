#pragma once

#include "vec2.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Implements a templated vector with 3 coordinates.
	 *
	 * @tparam T The type of the vector coordinates
	 */
	template<typename T>
	struct Vec3
	{
		/* Vec3 static values. */
		/// @{
		static const Vec3 zero;
		static const Vec3 one;
		static const Vec3 right;
		static const Vec3 up;
		static const Vec3 forward;
		static const Vec3 left;
		static const Vec3 down;
		static const Vec3 backward;
		/// @}

		union
		{
			/* Vector coordinates. */
			struct { T x, y, z; };

			/* Vector coordinates as an array. */
			T coords[3];
		};

		/**
		 * @brief Constructs a zero-initialized Vec3.
		 */
		constexpr FORCE_INLINE Vec3() : coords{} {}

		/**
		 * @brief Constructs a new Vec3 and set all coordinates equal to the
		 * given scalar value.
		 *
		 * @param s A scalar value
		 */
		constexpr FORCE_INLINE Vec3(T s) : coords{s, s, s} {}

		/**
		 * @brief Constructs a new Vec3 with the given coordinates.
		 *
		 * @param x,y,z The value of the coordinates
		 */
		constexpr FORCE_INLINE Vec3(T x, T y, T z) : coords{x, y, z} {}

		/**
		 * @brief Constructs a new Vec3 whose X and Y coordinates are equal to
		 * the given Vec2 and whose Z coordinate is zero-initialized.
		 *
		 * @param other A Vec2
		 */
		constexpr FORCE_INLINE explicit Vec3(Vec2<T> const& other)
			: coords{other.x, other.y}
		{}

		/**
		 * @brief Constructs a new Vec3 whose X and Y coordinates are equal to
		 * the given Vec2 and whose Z coordinate is equal to the given
		 * coordinate.
		 *
		 * @param other A Vec2
		 * @param z The value of the Z coordinate
		 */
		constexpr FORCE_INLINE Vec3(Vec2<T> const& other, T z)
			: coords{other.x, other.y, other.z}
		{}

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
			VW_CHECK(idx >= 0 && idx < 3);
			return coords[idx];
		}

		constexpr FORCE_INLINE T const& operator[](int idx) const
		{
			return const_cast<Vec3&>(*this)[idx];
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
		constexpr FORCE_INLINE Vec3& normalize()
		{
			return *this /= getSize();
		}

		/**
		 * @brief Returns a new Vec3 equal to this vector divided by its size.
		 *
		 * If the vector is not a floating-point vector, the result is
		 * undefined.
		 */
		constexpr FORCE_INLINE Vec3 getNormal() const
		{
			return *this / getSize();
		}

		/**
		 * @brief Returns a new Vec3 with same size but opposite direction.
		 */
		constexpr FORCE_INLINE Vec3 operator-() const
		{
			return {-x, -y, -z};
		}

		/**
		 * @brief Sums another vector to this vector, coordinate-wise.
		 *
		 * @param other Another Vec3
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec3& operator+=(Vec3 const& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		/**
		 * @brief Subtracts another vector from this vector, coordinate-wise.
		 *
		 * @param other Another Vec3
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec3& operator-=(Vec3 const& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		/**
		 * @brief Multiply this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec3
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec3& operator*=(Vec3 const& other)
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}

		/**
		 * @brief Divide this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec3
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec3& operator/=(Vec3 const& other)
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}

		/**
		 * @brief Returns the coordinate-wise sum of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u + v
		 */
		constexpr FORCE_INLINE Vec3 operator+(Vec3 const& other) const
		{
			return Vec3{*this} += other;
		}

		/**
		 * @brief Returns the coordinate-wise difference of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u - v
		 */
		constexpr FORCE_INLINE Vec3 operator-(Vec3 const& other) const
		{
			return Vec3{*this} -= other;
		}

		/**
		 * @brief Returns the coordinate-wise product of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u * v
		 */
		constexpr FORCE_INLINE Vec3 operator*(Vec3 const& other) const
		{
			return Vec3{*this} *= other;
		}

		/**
		 * @brief Returns the coordinate-wise division of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u / v
		 */
		constexpr FORCE_INLINE Vec3 operator/(Vec3 const& other) const
		{
			return Vec3{*this} /= other;
		}

		/**
		 * @brief Returns the sum of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec3
		 * @return <s, s, s> + v
		 */
		friend constexpr FORCE_INLINE Vec3 operator+(T const& s, Vec3 const& v)
		{
			return v + s;
		}

		/**
		 * @brief Returns the difference of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec3
		 * @return <s, s, s> - v
		 */
		friend constexpr FORCE_INLINE Vec3 operator-(T const& s, Vec3 const& v)
		{
			return {s - v.x, s - v.y, s - v.z};
		}

		/**
		 * @brief Returns the product of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec3
		 * @return <s, s, s> * v
		 */
		friend constexpr FORCE_INLINE Vec3 operator*(T const& s, Vec3 const& v)
		{
			return v * s;
		}

		/**
		 * @brief Returns the division of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec3
		 * @return <s, s, s> / v
		 */
		friend constexpr FORCE_INLINE Vec3 operator/(T const& s, Vec3 const& v)
		{
			return {s / v.x, s / v.y, s / v.z};
		}

		/**
		 * @brief Returns the dot product of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u dot v
		 */
		constexpr FORCE_INLINE T dot(Vec3 const& other) const
		{
			return x * other.x + y * other.y + z * other.z;
		}

		/**
		 * @brief Returns the cross product of two vectors.
		 *
		 * @param other Another Vec3
		 * @return u cross v
		 */
		constexpr FORCE_INLINE Vec3 cross(Vec3 const& other) const
		{
			return {y * other.z - z * other.y,
			        z * other.x - x * other.z,
					x * other.y - y * other.x};
		}

		/**
		 * @brief Converts this Vec3 to a Vec3 with different coordinates type.
		 */
		template<typename U>
		constexpr FORCE_INLINE explicit operator Vec3<U>() const
		{
			return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z)};
		}

		/**
		 * @brief Converts this Vec3 to a Vec2 by dropping the Z coordinate.
		 */
		constexpr FORCE_INLINE operator Vec2<T>() const
		{
			return {x, y};
		}
	};


	// ==================
	// Vec3 static values
	// ==================
	template<typename T>
	constexpr Vec3<T> Vec3<T>::zero = {};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::one = {1, 1, 1};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::right = {1, 0, 0};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::up = {0, 1, 0};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::forward = {0, 0, 1};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::left = {-1, 0, 0};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::down = {0, -1, 0};

	template<typename T>
	constexpr Vec3<T> Vec3<T>::backward = {0, 0, -1};
} // namespace VaporWorldVR::Math
