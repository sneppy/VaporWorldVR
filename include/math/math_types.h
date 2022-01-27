#pragma once

#include "core_types.h"


namespace VaporWorldVR::Math
{
	template<typename T> struct Vec2;
	template<typename T> struct Vec3;
	template<typename T> struct Vec4;
	                     struct Quat;
} // namespace VaporWorld::Math


namespace VaporWorldVR
{
	// =================
	// Math type aliases
	// =================
	using float2 = Math::Vec2<float>;
	using float3 = Math::Vec3<float>;
	using float4 = Math::Vec4<float>;

	using int2 = Math::Vec2<int>;
	using int3 = Math::Vec3<int>;
	using int4 = Math::Vec4<int>;

	using uint2 = Math::Vec2<uint>;
	using uint3 = Math::Vec3<uint>;
	using uint4 = Math::Vec4<uint>;

	using quat = Math::Quat;
} // namespace VaporWorld


#include "math_types.inl"
