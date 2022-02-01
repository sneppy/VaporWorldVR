#pragma once

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"


namespace VaporWorldVR
{
	/**
	 * @brief This struct holds informations about hit and intersection tests.
	 */
	struct HitResult
	{
		/* Flag set to false if no hit occured. */
		bool hitOccured = false;

		/* The position of the hit. */
		float3 hitPosition;

		/* The normal direction of the hit. */
		float3 hitNormal;

		/**
		 * @brief Returns true if a hit occured.
		 */
		constexpr FORCE_INLINE operator bool() const
		{
			return hitOccured;
		}
	};


	/**
	 * @brief Compute the intersection between a ray and a sphere.
	 *
	 * Note that If the ray starts inside the sphere, the closest hit result
	 * will be false.
	 *
	 * @param rayStart The starting point of the ray
	 * @param rayDir The direction of the ray (does not need to be normalized)
	 * @param sphereOrigin The position of the center of the sphere
	 * @param sphereRadius The length of the shere's radius
	 * @param[out] closestHit Returns the first hit. If the ray starts inside
	 *                        the sphere, this will be a non-hit
	 * @param[out] furthestHit Returns the second hit, or first if the ray
	 *                         starts inside the sphere
	 * @return true if at least one hit occured
	 * @return false otherwise
	 */
	bool raySphereIntersect(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                        float sphereRadius, HitResult& closestHit, HitResult& furthestHit);

	/**
	 * @brief Like raySphereIntersect(), but only returns the first actual hit.
	 *
	 * @param rayStart The starting point of the ray
	 * @param rayDir The direction of the ray (does not need to be normalized)
	 * @param sphereOrigin The position of the center of the sphere
	 * @param sphereRadius The length of the shere's radius
	 * @return The first actual hit, or no hit
	 */
	HitResult raySphereIntersect(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                             float sphereRadius);

	/**
	 * @brief Like raySphereIntersect(), but only returns a truth value.
	 *
	 * @param rayStart The starting point of the ray
	 * @param rayDir The direction of the ray (does not need to be normalized)
	 * @param sphereOrigin The position of the center of the sphere
	 * @param sphereRadius The length of the shere's radius
	 * @return true if at least one hit occured
	 * @return false otherwise
	 */
	bool raySphereIntersectTest(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                            float sphereRadius);

	/**
	 * @brief This method tests if a sphere overlaps with the given camera
	 * frustum.
	 *
	 * @param frustum The frustum matrix
	 * @param origin The position of center of the sphere
	 * @param radius The radius of the sphere. If not specified, the default
	 *               value is zero
	 * @return true if the sphere overlaps with the frustum
	 * @return false otherwise
	 */
	bool frustumSphereOverlapTest(float4x4 const& frustum, float3 const& origin, float radius = 0.f);

	/**
	 * @brief This method tests if a AABB object overlaps with the given camera
	 * frustum.
	 *
	 * The box overlaps if at least one of its vertices overlaps.
	 *
	 * @param frustum The frustum matrix
	 * @param min The first corner of the AABB to test
	 * @param max The second corner of the AABB to test
	 * @return true if the AABB overlaps with the frustum
	 * @return false otherwise
	 */
	bool frustumAABBOverlapTest(float4x4 const& frustum, float3 const& min, float3 const& max);
} // namespace VaporWorldVR
