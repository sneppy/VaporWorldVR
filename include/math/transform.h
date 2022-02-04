#pragma once

#include "mat4.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief A matrix used to describe a linear transformation.
	 */
	struct TransformationMatrix : public Mat4<float>
	{
		using Mat4::dot;

		/**
		 * @brief Construct a new TransformationMatrix with zero translation,
		 * zero rotation and uniform scale.
		 */
		constexpr FORCE_INLINE TransformationMatrix() : Mat4{Mat4::eye} {}

		/**
		 * @brief Construct a new TransformationMatrix with the given
		 * translation, zero rotation and uniform scale.
		 *
		 * @param translation The given translation
		 */
		constexpr FORCE_INLINE TransformationMatrix(Vec3<float> const& translation)
			: Mat4{1.f, 0.f, 0.f, translation.x,
			       0.f, 1.f, 0.f, translation.x,
			       0.f, 0.f, 1.f, translation.x,
			       0.f, 0.f, 0.f, 1.f}
		{}

		/**
		 * @brief Construct a new TransformationMatrix with the given
		 * translation, rotation and scale.
		 *
		 * @param translation The given translation
		 * @param rotation The given rotation
		 * @param scale The given scale
		 */
		constexpr TransformationMatrix(Vec3<float> const& translation, Quat const& rotation, Vec3<float> const& scale)
		{
			// Set rotation
			float const rx2 = rotation.x * rotation.x,
						rxy = rotation.x * rotation.y,
						rxz = rotation.x * rotation.z,
						rxw = rotation.x * rotation.w,
						ry2 = rotation.y * rotation.y,
						ryz = rotation.y * rotation.z,
						ryw = rotation.y * rotation.w,
						rz2 = rotation.z * rotation.z,
						rzw = rotation.z * rotation.w,
						rw2 = rotation.x * rotation.w;

			rows[0][0] = 1.f - 2.f * (ry2 + rz2);
			rows[0][1] = 2.f * (rxy - rzw);
			rows[0][2] = 2.f * (rxz + ryw);
			rows[1][0] = 2.f * (rxy + rzw);
			rows[1][1] = 1.f - 2.f * (rx2 + rz2);
			rows[1][2] = 2.f * (ryz - rxw);
			rows[2][0] = 2.f * (rxz - ryw);
			rows[2][1] = 2.f * (ryz + rxw);
			rows[2][2] = 1.f - 2.f * (rx2 + ry2);

			// Set scale
			for (int i = 0; i < 3; ++i)
			{
				rows[i].xyz = scale.dot(rows[i].xyz);
			}

			// Set translation
			rows[0][3] = translation[0];
			rows[1][3] = translation[1];
			rows[2][3] = translation[2];
		}

		/**
		 * @brief Returns the translation component of this transformation.
		 */
		constexpr FORCE_INLINE Vec3<float> getTranslation() const
		{
			return {rows[0][3], rows[1][3], rows[2][3]};
		}

		/**
		 * @brief Returns the rotation component of this transformation.
		 */
		constexpr Quat getRotation() const
		{
			Vec3<float> const invScale = 1.f / getScale();

			// https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
			float quatW = sqrt(1.f + rows[0][0] * invScale[0] + rows[1][1] * invScale[1] + rows[2][2] * invScale[2])
			              * 0.5f;
			float quatX = (rows[2][1] * invScale[1] - rows[1][2] * invScale[2]) / (4.f * quatW);
			float quatY = (rows[0][2] * invScale[2] - rows[2][0] * invScale[0]) / (4.f * quatW);
			float quatZ = (rows[1][0] * invScale[0] - rows[0][1] * invScale[1]) / (4.f * quatW);

			return {quatX, quatY, quatZ, quatW};
		}

		/**
		 * @brief Returns the squared scale component of this transformation.
		 */
		constexpr Vec3<float> getScale2() const
		{
			return {rows[0].xyz.getSize2(),
			        rows[1].xyz.getSize2(),
					rows[2].xyz.getSize2()};
		}

		/**
		 * @brief Returns the scale component of this transformation.
		 */
		constexpr Vec3<float> getScale() const
		{
			return {rows[0].xyz.getSize(),
			        rows[1].xyz.getSize(),
					rows[2].xyz.getSize()};
		}

		/**
		 * @brief Set the translation component of this transformation.
		 *
		 * @param translation The new translation component
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE TransformationMatrix& setTranslation(Vec3<float> const& translation)
		{
			rows[0][3] = translation[0];
			rows[1][3] = translation[1];
			rows[2][3] = translation[2];

			return *this;
		}

		/**
		 * @brief Set the rotation component of this transformation.
		 *
		 * @param rotation The new rotation component
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE TransformationMatrix& setRotation(Quat const& rotation)
		{
			Vec3<float> const scale = getScale();
			float const rx2 = rotation.x * rotation.x,
						rxy = rotation.x * rotation.y,
						rxz = rotation.x * rotation.z,
						rxw = rotation.x * rotation.w,
						ry2 = rotation.y * rotation.y,
						ryz = rotation.y * rotation.z,
						ryw = rotation.y * rotation.w,
						rz2 = rotation.z * rotation.z,
						rzw = rotation.z * rotation.w,
						rw2 = rotation.x * rotation.w;

			rows[0][0] = scale[0] * 1.f - 2.f * (ry2 + rz2);
			rows[0][1] = scale[1] * 2.f * (rxy - rzw);
			rows[0][2] = scale[2] * 2.f * (rxz + ryw);
			rows[1][0] = scale[0] * 2.f * (rxy + rzw);
			rows[1][1] = scale[1] * 1.f - 2.f * (rx2 + rz2);
			rows[1][2] = scale[2] * 2.f * (ryz - rxw);
			rows[2][0] = scale[0] * 2.f * (rxz - ryw);
			rows[2][1] = scale[1] * 2.f * (ryz + rxw);
			rows[2][2] = scale[2] * 1.f - 2.f * (rx2 + ry2);

			return *this;
		}

		/**
		 * @brief Set the scale component of this transformation.
		 *
		 * @param scale The new scale component
		 * @return Ref to self
		 */
		constexpr FORCE_INLINE TransformationMatrix& setScale(Vec3<float> const& scale)
		{
			Vec3<float> s = scale / getScale();
			for (int i = 0; i < 3; ++i)
			{
				rows[i][0] *= s[0];
				rows[i][1] *= s[1];
				rows[i][2] *= s[2];
			}

			return *this;
		}

		/**
		 * @brief Invert this transformation matrix.
		 */
		constexpr FORCE_INLINE TransformationMatrix& invert()
		{
			// Get the current scale
			Vec3<float> const invScale2 = 1.f / getScale2();

			// Transpose matrix to invert rotation
			transpose();

			// Reset last row to zero, which is now the translation row
			Vec3<float> const invTranslation = -rows[3].xyz;
			rows[3].xyz = {0.f, 0.f, 0.f};

			// Divide rotation rows by squared scale
			rows[0] /= invScale2[0];
			rows[1] /= invScale2[1];
			rows[2] /= invScale2[2];

			// Set translation
			rows[0][3] = invTranslation.dot(rows[0]);
			rows[1][3] = invTranslation.dot(rows[1]);
			rows[2][3] = invTranslation.dot(rows[2]);

			return *this;
		}

		/**
		 * @brief Returns the inverse of this matrix.
		 */
		constexpr TransformationMatrix operator!() const
		{
			return TransformationMatrix{*this}.invert();
		}

		/**
		 * @brief Compute the dot product between this transformation matrix and
		 * another transformation matrix.
		 *
		 * The result is another transformation matrix.
		 *
		 * @param other Another TransformationMatrix
		 * @return Dot product between this and another matrix
		 */
		constexpr FORCE_INLINE TransformationMatrix dot(TransformationMatrix const& other) const
		{
			return static_cast<TransformationMatrix&&>(Mat4::dot(other));
		}

	protected:
		using Mat4::Mat4;
	};
} // namespace VaporWorldVR::Math
