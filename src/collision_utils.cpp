#include "collision_utils.h"


namespace VaporWorldVR
{
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
} // namespace VaporWorldVR
