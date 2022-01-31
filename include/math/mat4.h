#pragma once

#include "vec4.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Row-major implementation of a 4x4 templated matrix.
	 *
	 * @tparam T The type of the elements of the matrix
	 */
	template<typename T>
	struct Mat4
	{
		/* Mat4 static values. */
		/// @{
		static const Mat4 zero;
		static const Mat4 eye;
		/// @}

		union
		{
			/* Row vectors. */
			Vec4<T> rows[4];

			/* Matrix plain data. */
			T data[16];
		};

		/**
		 * @brief Cosntructs a new Mat4 and zero-initialize all elements.
		 */
		constexpr FORCE_INLINE Mat4() : data{} {}

		/**
		 * @brief Constructs a new Mat4 with all elements equal to the given
		 * scalar value.
		 *
		 * @param s The scalar value
		 */
		constexpr FORCE_INLINE Mat4(T const& s)
			: data{s, s, s, s,
			       s, s, s, s,
			       s, s, s, s,
			       s, s, s, s}
		{}

		/**
		 * @brief Constructs a new Mat4 with the given values.
		 *
		 * @param a..p The values of the matrix
		 */
		constexpr FORCE_INLINE Mat4(T const& a, T const& b, T const& c, T const& d,
		                            T const& e, T const& f, T const& g, T const& h,
		                            T const& i, T const& j, T const& k, T const& l,
		                            T const& m, T const& n, T const& o, T const& p)
			: data{a, b, c, d,
			       e, f, g, h,
			       i, j, k, l,
			       m, n, o, p}
		{}

		/**
		 * @brief Constructs a new Mat4 with the given row vectors.
		 *
		 * @param i,j,k,l The row vectors
		 */
		constexpr FORCE_INLINE Mat4(Vec4<T> const& i,
		                            Vec4<T> const& j,
		                            Vec4<T> const& k,
		                            Vec4<T> const& l)
			: rows{i, j, k, l}
		{}

		/**
		 * @brief Return a ref to the i-th row vector.
		 *
		 * @param idx Index of the row vector
		 * @return Ref to i-th row vector
		 * @{
		 */
		constexpr FORCE_INLINE Vec4<T>& operator[](int idx)
		{
			VW_CHECK(idx >= 0 && idx < 4);
			return rows[idx];
		}

		constexpr FORCE_INLINE Vec4<T> const& operator[](int idx) const
		{
			return const_cast<Mat4&>(*this)[idx];
		}
		/// @}

		/**
		 * @brief Transpose this matrix in place.
		 */
		constexpr FORCE_INLINE Mat4& transpose()
		{
			return *this = getTransposed();
		}

		/**
		 * @brief Returns the transposed matrix.
		 */
		constexpr FORCE_INLINE Mat4 getTransposed() const
		{
			return {rows[0][0], rows[1][0], rows[2][0], rows[3][0],
			        rows[0][0], rows[1][0], rows[2][0], rows[3][0],
					rows[0][0], rows[1][0], rows[2][0], rows[3][0],
					rows[0][0], rows[1][0], rows[2][0], rows[3][0]};
		}

		/**
		 * @brief Invert this matrix in place.
		 */
		constexpr FORCE_INLINE Mat4& invert()
		{
			// Only implemented for floating-point types
			return *this = !(*this);
		}

		/**
		 * @brief Return the inverse of this matrix.
		 */
		constexpr FORCE_INLINE Mat4 operator!() const
		{
			// Only implemented for floating-point types
			return {};
		}

		/**
		 * @brief Return a Mat4 with all elements inverted.
		 */
		constexpr FORCE_INLINE Mat4 operator-() const
		{
			return {-data[0],  -data[1],  -data[2],  -data[3],
			        -data[4],  -data[5],  -data[6],  -data[7],
					-data[8],  -data[9],  -data[10], -data[11],
					-data[12], -data[13], -data[14], -data[15]};
		}

		/**
		 * @brief Sums the elements on another matrix to the elements of this
		 * matrix.
		 *
		 * @param other Another Mat4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Mat4& operator+=(Mat4 const& other)
		{
			for (int i = 0; i < 16; ++i)
			{
				data[i] += other.data[i];
			}
			return *this;
		}

		/**
		 * @brief Subtracts the elements on another matrix from the elements of
		 * this matrix.
		 *
		 * @param other Another Mat4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Mat4& operator-=(Mat4 const& other)
		{
			for (int i = 0; i < 16; ++i)
			{
				data[i] -= other.data[i];
			}
			return *this;
		}

		/**
		 * @brief Multiplies the elements on this matrix by the elements of
		 * another matrix.
		 *
		 * @param other Another Mat4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Mat4& operator*=(Mat4 const& other)
		{
			for (int i = 0; i < 16; ++i)
			{
				data[i] *= other.data[i];
			}
			return *this;
		}

		/**
		 * @brief Divides the elements on this matrix by the elements of another
		 * matrix.
		 *
		 * @param other Another Mat4
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE Mat4& operator/=(Mat4 const& other)
		{
			for (int i = 0; i < 16; ++i)
			{
				data[i] /= other.data[i];
			}
			return *this;
		}

		/**
		 * @brief Returns the element-wise sum of two matrices.
		 *
		 * @param other Another Mat4
		 * @return m + n
		 */
		constexpr FORCE_INLINE Mat4 operator+(Mat4 const& other) const
		{
			return Mat4{*this} += other;
		}

		/**
		 * @brief Returns the element-wise difference between two matrices.
		 *
		 * @param other Another Mat4
		 * @return m - n
		 */
		constexpr FORCE_INLINE Mat4 operator-(Mat4 const& other) const
		{
			return Mat4{*this} -= other;
		}

		/**
		 * @brief Returns the element-wise product of two matrices.
		 *
		 * @param other Another Mat4
		 * @return m - n
		 */
		constexpr FORCE_INLINE Mat4 operator*(Mat4 const& other) const
		{
			return Mat4{*this} *= other;
		}

		/**
		 * @brief Returns the element-wise division of two matrices.
		 *
		 * @param other Another Mat4
		 * @return m - n
		 */
		constexpr FORCE_INLINE Mat4 operator/(Mat4 const& other) const
		{
			return Mat4{*this} /= other;
		}

		/**
		 * @brief Compute the dot product between two matrices.
		 *
		 * @param other Another Mat4
		 * @return m dot n
		 */
		constexpr FORCE_INLINE Mat4 dot(Mat4 const& other) const
		{
			return Mat4{*this}.dot_Impl(other.getTranposed());
		}

		/**
		 * @brief Compute the dot product between this matrix and a Vec4.
		 *
		 * @param v A Vec4
		 * @return m dot v
		 */
		constexpr FORCE_INLINE Vec4<T> dot(Vec4<T> const& v) const
		{
			return {rows[0].dot(v), rows[1].dot(v), rows[2].dot(v), rows[3].dot(v)};
		}

		/**
		 * @brief Apply the transformation described by this matrix to a
		 * position vector.
		 *
		 * @param v The Vec3 to transform
		 * @return M(v)
		 */
		constexpr FORCE_INLINE Vec3<T> transformVector(Vec3<T> const& v) const
		{
			// Compiler should automatically drop the W coordinate
			return dot({v, 1});
		}

	protected:
		/* Internal implementation of the dot product, the parameter is the
		   transposed matrix operand. */
		constexpr Mat4& dot_Impl(Mat4 const& otherT)
		{
			for (int i = 0; i < 4; ++i)
			{
				rows[i] = {rows[i].dot(otherT[0]),
				           rows[i].dot(otherT[1]),
						   rows[i].dot(otherT[2]),
				           rows[i].dot(otherT[3])};
			}
			return *this;
		}

		/* Returns the matrix of the algebraic components. */
		constexpr Mat4 getComplementsMatrix() const
		{
			T const afbe = rows[0][0] * rows[1][1] - rows[0][1] * rows[1][0],
			        agce = rows[0][0] * rows[1][2] - rows[0][2] * rows[1][0],
			        ahde = rows[0][0] * rows[1][3] - rows[0][3] * rows[1][0],
			        bgcf = rows[0][1] * rows[1][2] - rows[0][2] * rows[1][1],
			        bhdf = rows[0][1] * rows[1][3] - rows[0][3] * rows[1][1],
			        chdg = rows[0][2] * rows[1][3] - rows[0][3] * rows[1][2],
			        injm = rows[2][0] * rows[3][1] - rows[2][1] * rows[3][0],
			        iokm = rows[2][0] * rows[3][2] - rows[2][2] * rows[3][0],
			        iplm = rows[2][0] * rows[3][3] - rows[2][3] * rows[3][0],
			        jokn = rows[2][1] * rows[3][2] - rows[2][2] * rows[3][1],
			        jpln = rows[2][1] * rows[3][3] - rows[2][3] * rows[3][1],
			        kplo = rows[2][2] * rows[3][3] - rows[2][3] * rows[3][2];

			return {+ rows[1][1] * kplo - rows[1][2] * jpln + rows[1][3] * jokn,
			        - rows[1][0] * kplo + rows[1][2] * iplm - rows[1][3] * iokm,
			        + rows[1][0] * jpln - rows[1][1] * iplm + rows[1][3] * injm,
			        - rows[1][0] * jokn + rows[1][1] * iokm - rows[1][2] * injm,

			        - rows[0][1] * kplo + rows[0][2] * jpln - rows[0][3] * jokn,
			        + rows[0][0] * kplo - rows[0][2] * iplm + rows[0][3] * iokm,
			        - rows[0][0] * jpln + rows[0][1] * iplm - rows[0][3] * injm,
			        + rows[0][0] * jokn - rows[0][1] * iokm + rows[0][2] * injm,

			        + rows[3][1] * chdg - rows[3][2] * bhdf + rows[3][3] * bgcf,
			        - rows[3][0] * chdg + rows[3][2] * ahde - rows[3][3] * agce,
			        + rows[3][0] * bhdf - rows[3][1] * ahde + rows[3][3] * afbe,
			        - rows[3][0] * bgcf + rows[3][1] * agce - rows[3][2] * afbe,

			        - rows[2][1] * chdg + rows[2][2] * bhdf - rows[2][3] * bgcf,
			        + rows[2][0] * chdg - rows[2][2] * ahde + rows[2][3] * agce,
			        - rows[2][0] * bhdf + rows[2][1] * ahde - rows[2][3] * afbe,
			        + rows[2][0] * bgcf - rows[2][1] * agce + rows[2][2] * afbe};
		}
	};


	// ==================================
	// Floating-point Mat4 specialization
	// ==================================
	template<>
	constexpr FORCE_INLINE Mat4<float> Mat4<float>::operator!() const
	{
		Mat4<float> compMatrix = getComplementsMatrix();
		float invDet = 1.f / compMatrix[0].dot(rows[0]);
		return compMatrix.transpose() * invDet;
	}

	template<>
	constexpr FORCE_INLINE Mat4<double> Mat4<double>::operator!() const
	{
		Mat4<double> compMatrix = getComplementsMatrix();
		double invDet = 1.f / compMatrix[0].dot(rows[0]);
		return compMatrix.transpose() * invDet;
	}


	// ==================
	// Vec4 static values
	// ==================
	template<typename T>
	constexpr Mat4<T> Mat4<T>::zero = {};

	template<typename T>
	constexpr Mat4<T> Mat4<T>::eye = {1, 0, 0, 0,
	                                  0, 1, 0, 0,
									  0, 0, 1, 0,
									  0, 0, 0, 1};
} // namespace VaporWorldVR::Math
