#include "collision_utils.h"

#include <arm_neon.h>


namespace VaporWorldVR
{
	namespace
	{
		/* Returns the planes of the frustum given the transposed frustum
		   matrix. */
		static void getFrustumTPlanes(float4 planes[], float4x4 const& frustumT)
		{
			planes[0] = frustumT[3] + frustumT[0];
			planes[1] = frustumT[3] - frustumT[0];
			planes[2] = frustumT[3] + frustumT[1];
			planes[3] = frustumT[3] - frustumT[1];
			planes[4] = frustumT[3];
			planes[5] = frustumT[3] - frustumT[2];
		}

		/* Returns the vertices of the given AABB. */
		static void getAAABVertices(float3 vertices[], float3 const& min, float3 const& max)
		{
			float3 extent = max - min;
			vertices[0] = min;
			vertices[1] = min + float3{extent.x, 0.f, 0.f};
			vertices[2] = min + float3{0.f, extent.y, 0.f};
			vertices[3] = min + float3{extent.x, extent.y, 0.f};
			vertices[4] = max - float3{extent.x, extent.y, 0.f};
			vertices[5] = max - float3{0.f, extent.y, 0.f};
			vertices[6] = max - float3{extent.x, 0.f, 0.f};
			vertices[7] = max;
		}

		/* Returns true if the given point is within the frustum defined by the
		   given planes. The order of the planes does not matter. */
		static bool frustumTest_Impl(float4 const planes[], float3 const& pos, float radius)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (pos.dot(planes[i].xyz) + planes[i].w + radius < 0.f)
					// Point is outside of frustum
					return false;
			}

			return true;
		}
	} // namespace


	bool raySphereIntersect(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                        float sphereRadius, HitResult& closestHit, HitResult& furthestHit)
	{
		// Solve the quadratic equation dist(rayStart + rayDir * x, spherOrigin) = spherRadius
		float3 sphereToRay = sphereOrigin - rayStart;
		float a = rayDir.dot(rayDir);
		float b2 = rayDir.dot(sphereToRay);
		float c = sphereToRay.dot(sphereToRay) - (sphereRadius * sphereRadius);
		float d = b2 * b2 - a * c;

		if (d < 0.f)
		{
			// No real solution, no intersection
			return false;
		}

		// Get first intersection point
		float x0 = (b2 - Math::sqrt(d)) / a;
		closestHit.hitOccured = x0 >= 0.f;
		closestHit.hitPosition = rayStart + rayDir * x0;
		closestHit.hitNormal = (closestHit.hitPosition - sphereOrigin).normalize();

		// Get second intersection point
		float x1 = (b2 + Math::sqrt(d)) / a;
		furthestHit.hitOccured = x1 >= 0.f;
		furthestHit.hitPosition = rayStart + rayDir * x0;
		furthestHit.hitNormal = (furthestHit.hitPosition - sphereOrigin).normalize();

		return true;
	}

	inline HitResult raySphereIntersect(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                                    float sphereRadius)
	{
		if (HitResult hitResults[2]; raySphereIntersect(rayStart, rayDir, sphereOrigin, sphereRadius, hitResults[0],
		                                                hitResults[1]))
		{
			// Closest hit may actually be a false hit
			return hitResults[0] ? hitResults[0] : hitResults[1];
		}

		// No hit occured
		return {};
	}

	inline bool raySphereIntersectTest(float3 const& rayStart, float3 const& rayDir, float3 const& sphereOrigin,
	                                   float sphereRadius)
	{
		if ((rayStart - sphereOrigin).getSize2() <= sphereRadius * sphereRadius)
		{
			// If ray starts inside, then ray and sphere intersect
			return true;
		}

		// Test intersection
		HitResult _[2];
		return raySphereIntersect(rayStart, rayDir, sphereOrigin, sphereRadius, _[0], _[1]);
	}

	bool frustumSphereOverlapTest(float4x4 const& frustum, float3 const& origin, float radius)
	{
		// Get frustum planes
		float4 planes[6];
		getFrustumTPlanes(planes, frustum.getTransposed());
		return frustumTest_Impl(planes, origin, radius);
	}

	bool frustumAABBOverlapTest(float4x4 const& frustum, float3 const& min, float3 const& max)
	{
		// Get AABB vertices
		float3 vertices[8];
		getAAABVertices(vertices, min, max);

		// Get frustum planes
		float4 planes[6];
		getFrustumTPlanes(planes, frustum.getTransposed());

		for (int i = 0; i < 8; ++i)
		{
			if (frustumTest_Impl(planes, vertices[0], 0.f))
				// At least one vertex overlaps
				return true;
		}

		return false;
	}
} // namespace VaporWorldVR
