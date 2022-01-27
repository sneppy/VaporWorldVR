#pragma once

#include "vec3.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Implements a templated vector with 4 coordinates.
	 *
	 * @tparam T The type of the vector coordinates
	 */
	template<typename T>
	struct Vec4
	{
		/* Vec4 static values. */
		/// @{
		static const Vec4 zero;
		static const Vec4 one;
		/// @}

		union
		{
			/* Vector space coordinates. */
			struct { T x, y, z, w; };

			/* Vector coordinates array. */
			T coords[4];
		};

		/**
		 * @brief Constructs a zero-initialized Vec4.
		 */
		constexpr FORCE_INLINE Vec4() : coords{} {}

		/**
		 * @brief Constructs a new Vec4 and set all coordinates equal to the
		 * given scalar value.
		 *
		 * @param s A scalar value
		 */
		constexpr FORCE_INLINE Vec4(T s) : coords{s, s, s, s} {}

		/**
		 * @brief Constructs a new Vec4 with the given coordinates.
		 *
		 * @param x,y,z The value of the coordinates
		 */
		constexpr FORCE_INLINE Vec4(T x, T y, T z, T w) : coords{x, y, z, w} {}

		/**
		 * @brief Constructs a new Vec4 whose X, Y and Z coordinates are equal
		 * to the given Vec3 and whose W coordinate is zero-initialized.
		 *
		 * @param other A Vec3
		 */
		constexpr FORCE_INLINE explicit Vec4(Vec3<T> const& other)
			: coords{other.x, other.y, other.z}
		{}

		/**
		 * @brief Constructs a new Vec4 whose X, Y and Z coordinates are equal
		 * to the given Vec3 and whose W coordinate is equal to the given
		 * coordinate.
		 *
		 * @param other A Vec3
		 * @param z The value of the W coordinate
		 */
		constexpr FORCE_INLINE Vec4(Vec3<T> const& other, T z)
			: coords{other.x, other.y, other.z, w}
		{}

		/**
		 * @brief Returns a ref to the coordinate at the given position.
		 *
		 * If the position is out of range, the result is undefined.
		 *
		 * @param idx Position of the coordinate.
		 * @return Ref to i-th coordinate
		 * @{
		 */
		constexpr FORCE_INLINE T& operator[](int idx)
		{
			VW_CHECK(idx >= 0 && idx < 4);
			return coords[idx];
		}

		constexpr FORCE_INLINE T const& operator[](int idx) const
		{
			return const_cast<Vec4&>(*this)[idx];
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
		 * @brief Same as getSize(), but uses the fast version of sqrt.
		 *
		 * The return value is much less accurate then the return of getSize(),
		 * so use carefully.
		 */
		constexpr FORCE_INLINE T getSize_Fast() const
		{
			// Only defined for 32-bit float coordinates vector
			return {};
		}

		/**
		 * @brief Divides all coordinates by the vector's size.
		 *
		 * If the vector is not a floating-point vector, the result is
		 * undefined.
		 */
		constexpr FORCE_INLINE Vec4& normalize()
		{
			return *this /= getSize();
		}

		/**
		 * @brief Returns a new Vec4 equal to this vector divided by its size.
		 *
		 * If the vector is not a floating-point vector, the result is
		 * undefined.
		 */
		constexpr FORCE_INLINE Vec4 getNormal() const
		{
			return *this / getSize();
		}

		/**
		 * @brief Returns a new Vec4 with same size but opposite direction.
		 */
		constexpr FORCE_INLINE Vec4 operator-() const
		{
			return {-x, -y, -z};
		}

		/**
		 * @brief Sums another vector to this vector, coordinate-wise.
		 *
		 * @param other Another Vec4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec4& operator+=(Vec4 const& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}

		/**
		 * @brief Subtracts another vector from this vector, coordinate-wise.
		 *
		 * @param other Another Vec4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec4& operator-=(Vec4 const& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}

		/**
		 * @brief Multiply this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec4& operator*=(Vec4 const& other)
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			w *= other.w;
			return *this;
		}

		/**
		 * @brief Divide this vector by another vector, coordinate-wise.
		 *
		 * @param other Another Vec4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Vec4& operator/=(Vec4 const& other)
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			w /= other.w;
			return *this;
		}

		/**
		 * @brief Returns the coordinate-wise sum of two vectors.
		 *
		 * @param other Another Vec4
		 * @return u + v
		 */
		constexpr FORCE_INLINE Vec4 operator+(Vec4 const& other) const
		{
			return Vec4{*this} += other;
		}

		/**
		 * @brief Returns the coordinate-wise difference of two vectors.
		 *
		 * @param other Another Vec4
		 * @return u - v
		 */
		constexpr FORCE_INLINE Vec4 operator-(Vec4 const& other) const
		{
			return Vec4{*this} -= other;
		}

		/**
		 * @brief Returns the coordinate-wise product of two vectors.
		 *
		 * @param other Another Vec4
		 * @return u * v
		 */
		constexpr FORCE_INLINE Vec4 operator*(Vec4 const& other) const
		{
			return Vec4{*this} *= other;
		}

		/**
		 * @brief Returns the coordinate-wise division of two vectors.
		 *
		 * @param other Another Vec4
		 * @return u / v
		 */
		constexpr FORCE_INLINE Vec4 operator/(Vec4 const& other) const
		{
			return Vec4{*this} /= other;
		}

		/**
		 * @brief Returns the sum of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec4
		 * @return <s, s, s, s> + v
		 */
		friend constexpr FORCE_INLINE Vec4 operator+(T const& s, Vec4 const& v)
		{
			return v + s;
		}

		/**
		 * @brief Returns the difference of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec4
		 * @return <s, s, s, s> - v
		 */
		friend constexpr FORCE_INLINE Vec4 operator-(T const& s, Vec4 const& v)
		{
			return {s - v.x, s - v.y, s - v.z, s - v.w};
		}

		/**
		 * @brief Returns the product of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec4
		 * @return <s, s, s> * v
		 */
		friend constexpr FORCE_INLINE Vec4 operator*(T const& s, Vec4 const& v)
		{
			return v * s;
		}

		/**
		 * @brief Returns the division of a scalar vector with another vector.
		 *
		 * @param s A scalar value
		 * @param other A Vec4
		 * @return <s, s, s> / v
		 */
		friend constexpr FORCE_INLINE Vec4 operator/(T const& s, Vec4 const& v)
		{
			return {s / v.x, s / v.y, s / v.z, s / v.w};
		}

		/**
		 * @brief Returns the dot product of two vectors.
		 *
		 * @param other Another Vec4
		 * @return u dot v
		 */
		constexpr FORCE_INLINE T dot(Vec4 const& other) const
		{
			return x * other.x + y * other.y + z * other.z + w * other.w;
		}

		/**
		 * @brief Converts this Vec4 to a Vec4 with different coordinates type.
		 */
		template<typename U>
		constexpr FORCE_INLINE explicit operator Vec4<U>() const
		{
			return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w)};
		}

		/**
		 * @brief Converts this Vec4 to a Vec3 by dropping the Z coordinate.
		 */
		constexpr FORCE_INLINE operator Vec3<T>() const
		{
			return {x, y, z};
		}
	};


	// ==================================
	// Vec4 floating-point specialization
	// ==================================
	template<>
	constexpr FORCE_INLINE float Vec4<float>::getSize() const
	{
		return sqrt(getSize2());
	}

	template<>
	constexpr FORCE_INLINE double Vec4<double>::getSize() const
	{
		return sqrt(getSize2());
	}

	template<>
	constexpr FORCE_INLINE float Vec4<float>::getSize_Fast() const
	{
		return fsqrt(getSize2());
	}


	// ==================
	// Vec4 static values
	// ==================
	template<typename T>
	constexpr Vec4<T> Vec4<T>::zero = {};

	template<typename T>
	constexpr Vec4<T> Vec4<T>::one = {1, 1, 1, 1};
} // namespace VaporWorldVR::Math
