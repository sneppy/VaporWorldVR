#pragma once

#include "vec4.h"


namespace VaporWorldVR::Math
{
	/**
	 * @brief Implements a quaternion used to decribe a rotation in 3D space.
	 */
	struct Quat : Vec4<float>
	{
		/**
		 * @brief Constructs a new Quat describing a zero-rotation.
		 */
		constexpr FORCE_INLINE Quat() : Vec4{0.f, 0.f, 0.f, 1.f} {}

		/**
		 * @brief Constructs a new Quat describing a rotation of the given
		 * angle around the given axis.
		 *
		 * @param axis The axis of the rotation
		 * @param angle The angle of the rotation
		 */
		constexpr FORCE_INLINE Quat(Vec3<float> const& axis, float angle)
		{
			// TODO: Check axis is normalized
			float c = cos(angle * 0.5f);
			float s = sin(angle * 0.5f);

			x = axis.x * s;
			y = axis.y * s;
			z = axis.z * s;
			w = c;
		}

		/**
		 * @brief Constructs a new Quat that describes a rotation around the
		 * given rotation vector with an angle proportional to the size of the
		 * rotation vector.
		 *
		 * @param rotVector The rotation vector
		 */
		constexpr FORCE_INLINE explicit Quat(Vec3<float> const& rotVector)
		{
			float angle = rotVector.getSize();
			float c = cos(angle * 0.5f);
			float s = sin(angle * 0.5f) / angle;

			x = rotVector.x * s;
			y = rotVector.y * s;
			z = rotVector.z * s;
			w = c;
		}

		/**
		 * @brief Returns the angle of the rotation.
		 */
		constexpr FORCE_INLINE float getAngle() const
		{
			return acos(w) * 2.f;
		}

		/**
		 * @brief Returns the axis of rotation.
		 */
		constexpr FORCE_INLINE Vec3<float> getAxis() const
		{
			return Vec3<float>{*this}.normalize();
		}

		/**
		 * @brief Returns the axis and angle of rotation.
		 *
		 * Always prefer this method if you need to fully describe the rotation.
		 *
		 * @param outAxis The return axis of the rotation
		 * @param outAngle The return angle of the rotation
		 */
		constexpr FORCE_INLINE void getAxisAndAngle(Vec3<float>& outAxis, float& outAngle) const
		{
			Vec3<float> axis{*this};
			float s = axis.getSize();

			outAngle = atan2(s, w) * 2.f;
			outAxis = axis / s;
		}

		/**
		 * @brief Returns a new Quat that describes a rotation around the same
		 * axis but inverse angle.
		 */
		constexpr FORCE_INLINE Quat operator!() const
		{
			return {x, y, z, -w};
		}

		/**
		 * @brief Return a new Quat that describe the combined rotation of this
		 * Quat with another Quat.
		 *
		 * @param other Another Quat
		 * @return The combined rotation
		 */
		constexpr FORCE_INLINE Quat operator*(Quat const& other) const
		{
			return {x * other.w + y * other.z - z * other.y + w * other.x,
			        -x * other.z + y * other.w + z * other.x + w * other.y,
					x * other.y - y * other.x + z * other.w + w * other.z,
			        -x * other.x - y * other.y - z * other.z + w * other.w};
		}

		/**
		 * @brief Rotate the given vector by this Quat.
		 *
		 * @param v A Vec3
		 * @return Rotated vector
		 */
		constexpr FORCE_INLINE Vec3<float> rotateVector(Vec3<float> const& v) const
		{
			// http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
			const Vec3<float> q{*this};
			const Vec3<float> t = (q.cross(v)) * 2.f;
			return v + (t * w) + (q.cross(t));
		}

	protected:
		using Vec4::Vec4;
	};
} // namespace VaporWorldVR::Math
