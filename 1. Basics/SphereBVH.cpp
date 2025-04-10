#include "precomp.h"
#include "SphereBVH.h"

SphereBVH::SphereBVH(const float3 _pos, const float _r, const int _material, const int _texture)
{
	m_spheres = new Sphere[1];
	m_spheres[0] = {_pos, _r, _texture, _material};

	m_bvh.Build([&](unsigned int _a, float3& _b, float3& _c) { SphereAabb(_a, _b, _c); }, 1);
	m_bvh.customIntersect = [&](tinybvh::Ray& _ray, const unsigned _primId) { return SphereIntersect(_ray, _primId); };
	m_bvh.customIsOccluded = [&](const tinybvh::Ray& _ray, const unsigned _primId)
	{
		return SphereIsOccluded(_ray, _primId);
	};
}

SphereBVH::SphereBVH(const Sphere* _setSpheres, const uint _amount)
{
	m_spheres = new Sphere[_amount];
	for (uint i = 0; i < _amount; i++)
	{
		m_spheres[i] = _setSpheres[i];
	}

	m_bvh.Build([&](unsigned int _a, float3& _b, float3& _c) { SphereAabb(_a, _b, _c); }, _amount);
	m_bvh.customIntersect = [&](tinybvh::Ray& _ray, const unsigned _primId) { return SphereIntersect(_ray, _primId); };
	m_bvh.customIsOccluded = [&](const tinybvh::Ray& _ray, const unsigned _primId)
	{
		return SphereIsOccluded(_ray, _primId);
	};
}

SphereBVH::~SphereBVH()
{
	delete[] m_spheres;
}

//From Tinybvh > tinybvh_custom.h
bool SphereBVH::SphereIntersect(tinybvh::Ray& _ray, const unsigned _primId) const
{
	const float r = m_spheres[_primId].r;
	if (r <= 0) return false;
	float rayLength = length(_ray.D);
	float3 D = _ray.D / rayLength;
	float rayT = _ray.hit.t * rayLength;
	const float3 oc = _ray.O - m_spheres[_primId].pos;
	const float b = dot(oc, D);
	const float c = dot(oc, oc) - r * r;
	float t, d = b * b - c;
	if (d <= 0) return false;
	d = sqrtf(d);
	const float t1 = -b - d;
	const float t2 = -b + d;
	if (t1 > 0)
	{
		t = t1;
	}
	else
	{
		t = t2;
		if (t < 0) return false; // The sphere is behind the ray origin
	}
	// Check if this intersection is closer than the current hit
	const bool hit = t < rayT && t > 0;
	if (hit)
	{
		_ray.hit.t = t;
		_ray.hit.prim = _primId;
	}

	return hit;
}

bool SphereBVH::SphereIsOccluded(const tinybvh::Ray& _ray, const unsigned _primId) const
{
	const float r = m_spheres[_primId].r;
	if (r <= 0) return false;

	float rayLength = length(_ray.D);
	const float3 D = _ray.D / rayLength;

	const float3 oc = _ray.O - m_spheres[_primId].pos;
	const float b = dot(oc, D);
	const float c = dot(oc, oc) - r * r;
	float t, d = b * b - c;
	if (d <= 0) return false;
	d = sqrtf(d);
	const float t1 = -b - d;
	const float t2 = -b + d;
	if (t1 > 0)
	{
		t = t1;
	}
	else
	{
		t = t2;
		if (t < 0) return false; // The sphere is behind the ray origin
	}
	const float rayT = _ray.hit.t * rayLength;
	return t < rayT && t > 0;
}

void SphereBVH::SphereAabb(const unsigned int _primId, float3& _boundsMin, float3& _boundsMax) const
{
	_boundsMin = m_spheres[_primId].pos - float3(m_spheres[_primId].r);
	_boundsMax = m_spheres[_primId].pos + float3(m_spheres[_primId].r);
}
