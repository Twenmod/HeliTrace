#include "precomp.h"
#include "CubeBVH.h"

CubeBVH::CubeBVH(mat4 _transform, float3 _halfSize, int _material, int _texture)
{
	m_cubes = new Cube[1];

	float4 min = -_halfSize;
	float4 max = _halfSize;
	m_cubes[0].bmin4 = _mm_load_ps(&min.x);
	m_cubes[0].bmax4 = _mm_load_ps(&max.x);
	m_cubes[0].transform = _transform;
	m_cubes[0].invTransform = _transform.Inverted();
	m_cubes[0].material = _material;
	m_cubes[0].texture = _texture;

	m_bvh.Build([&](unsigned int a, float3& b, float3& c) { CubeAABB(a, b, c); }, 1);
	m_bvh.customIntersect = [&](tinybvh::Ray& ray, const unsigned primID) { return CubeIntersect(ray, primID); };
	m_bvh.customIsOccluded = [&](const tinybvh::Ray& ray, const unsigned primID)
	{
		return CubeIsOccluded(ray, primID);
	};

}

CubeBVH::CubeBVH(Cube* setCubes, uint _amount)
{
	m_cubes = new Cube[_amount];
	for (uint i = 0; i < _amount; i++)
	{
		m_cubes[i] = setCubes[i];
	}

	m_bvh.Build([&](unsigned int a, float3& b, float3& c) { CubeAABB(a, b, c); }, _amount);
	m_bvh.customIntersect = [&](tinybvh::Ray& ray, const unsigned primID) { return CubeIntersect(ray, primID); };
	m_bvh.customIsOccluded = [&](const tinybvh::Ray& ray, const unsigned primID)
	{
		return CubeIsOccluded(ray, primID);
	};
}

CubeBVH::~CubeBVH()
{
	delete[] m_cubes;
}

bool CubeBVH::CubeIntersect(tinybvh::Ray& _ray, const unsigned _primID)
{
	tinybvh::Ray transRay;
	transRay.O = float3(float4(_ray.O, 1.f) * m_cubes[_primID].invTransform);
	transRay.D = float3(float4(_ray.D, 0.f) * m_cubes[_primID].invTransform);
	transRay.rD = 1.f/normalize(transRay.D);

	__m128 O4 = _mm_load_ps(&transRay.O.x);
	__m128 rD4 = _mm_load_ps(&transRay.rD.x);

	//Code from scene.h

	// AABB test
	__m128 t1 = _mm_mul_ps(_mm_sub_ps(m_cubes[_primID].bmin4, O4), rD4);
	__m128 t2 = _mm_mul_ps(_mm_sub_ps(m_cubes[_primID].bmax4, O4), rD4);
	__m128 vmax4 = _mm_max_ps(t1, t2), vmin4 = _mm_min_ps(t1, t2);
	float tmax = min(vmax4.m128_f32[0], min(vmax4.m128_f32[1], vmax4.m128_f32[2]));
	float tmin = max(vmin4.m128_f32[0], max(vmin4.m128_f32[1], vmin4.m128_f32[2]));
	if (tmin < tmax) if (tmin > 0)
	{
		if (tmin < _ray.hit.t)
		{
			_ray.hit.t = tmin, _ray.hit.prim = _primID;
			return true;
		}
	}
	else if (tmax > 0)
	{
		if (tmax < _ray.hit.t)
		{
			_ray.hit.t = tmax, _ray.hit.prim = _primID;
			return true;
		}
	}

	return false;
}

bool CubeBVH::CubeIsOccluded(const tinybvh::Ray& _ray, const unsigned _primID)
{

	tinybvh::Ray transRay;
	transRay.O = float3(float4(_ray.O, 1.f) * m_cubes[_primID].invTransform);
	transRay.D = float3(float4(_ray.D, 0.f) * m_cubes[_primID].invTransform);
	transRay.rD = 1.f / normalize(transRay.D);

	__m128 O4 = _mm_load_ps(&transRay.O.x);
	__m128 rD4 = _mm_load_ps(&transRay.rD.x);

	// AABB test
	__m128 t1 = _mm_mul_ps(_mm_sub_ps(m_cubes[_primID].bmin4, O4), rD4);
	__m128 t2 = _mm_mul_ps(_mm_sub_ps(m_cubes[_primID].bmax4, O4), rD4);
	__m128 vmax4 = _mm_max_ps(t1, t2), vmin4 = _mm_min_ps(t1, t2);
	float tmax = min(vmax4.m128_f32[0], min(vmax4.m128_f32[1], vmax4.m128_f32[2]));
	float tmin = max(vmin4.m128_f32[0], max(vmin4.m128_f32[1], vmin4.m128_f32[2]));
	return tmax > 0 && tmin < tmax && tmin < _ray.hit.t;
}

void CubeBVH::CubeAABB(unsigned int _primID, float3& _boundsMin, float3& _boundsMax)
{
	// Extract AABB min and max corners
	float4 bmin = float4(m_cubes[_primID].bmin4.m128_f32[0], m_cubes[_primID].bmin4.m128_f32[1],
	                     m_cubes[_primID].bmin4.m128_f32[2], 1.f);
	float4 bmax = float4(m_cubes[_primID].bmax4.m128_f32[0], m_cubes[_primID].bmax4.m128_f32[1],
	                     m_cubes[_primID].bmax4.m128_f32[2], 1.f);

	// Apply forward transform to all 8 corners of the AABB
	float4 corners[8] = {
		float4(bmin.x, bmin.y, bmin.z, 1.f),
		float4(bmax.x, bmin.y, bmin.z, 1.f),
		float4(bmin.x, bmax.y, bmin.z, 1.f),
		float4(bmin.x, bmin.y, bmax.z, 1.f),
		float4(bmax.x, bmax.y, bmin.z, 1.f),
		float4(bmax.x, bmin.y, bmax.z, 1.f),
		float4(bmin.x, bmax.y, bmax.z, 1.f),
		float4(bmax.x, bmax.y, bmax.z, 1.f)
	};

	// Transform all corners
	for (int i = 0; i < 8; ++i)
	{
		corners[i] = corners[i] * m_cubes[_primID].transform;
	}

	// Initialize boundsMin and boundsMax to the first transformed corner
	_boundsMin = float3(corners[0].x, corners[0].y, corners[0].z);
	_boundsMax = float3(corners[0].x, corners[0].y, corners[0].z);

	// Find new AABB bounds
	for (int i = 1; i < 8; ++i)
	{
		_boundsMin = float3(
			min(_boundsMin.x, corners[i].x),
			min(_boundsMin.y, corners[i].y),
			min(_boundsMin.z, corners[i].z)
		);
		_boundsMax = float3(
			max(_boundsMax.x, corners[i].x),
			max(_boundsMax.y, corners[i].y),
			max(_boundsMax.z, corners[i].z)
		);
	}
}


