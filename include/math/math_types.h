#pragma once

#include "core_types.h"


namespace VaporWorldVR::Math
{
	template<typename T> struct Vec2;
	template<typename T> struct Vec3;
	template<typename T> struct Vec4;
	                     struct Quat;
	template<typename T> struct Mat4;
} // namespace VaporWorld::Math


namespace VaporWorldVR
{
	// =================
	// Math type aliases
	// =================
	using float2   = Math::Vec2<float>;
	using float3   = Math::Vec3<float>;
	using float4   = Math::Vec4<float>;
	using float4x4 = Math::Mat4<float>;

	using int2   = Math::Vec2<int>;
	using int3   = Math::Vec3<int>;
	using int4   = Math::Vec4<int>;
	using int4x4 = Math::Mat4<int>;

	using uint2   = Math::Vec2<uint32_t>;
	using uint3   = Math::Vec3<uint32_t>;
	using uint4   = Math::Vec4<uint32_t>;
	using uint4x4 = Math::Mat4<uint32_t>;

	using quat = Math::Quat;
} // namespace VaporWorld


#include "math_types.inl"
