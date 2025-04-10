#pragma once
struct Sphere
{
	float3 pos{float3(0.f)};
	float r{0.f};
	int texture = -1;
	int material = 0;
};

class SphereBVH
{
public:
	SphereBVH(float3 _pos, float _r, int _material, int _texture = -1);
	SphereBVH(const Sphere* _spheres, uint _amount);

	~SphereBVH();
	tinybvh::BVH m_bvh;
	Sphere* m_spheres;

protected:
	bool SphereIntersect(tinybvh::Ray& _ray, const unsigned _primId) const;
	bool SphereIsOccluded(const tinybvh::Ray& _ray, const unsigned _primID) const;
	void SphereAabb(unsigned int _primId, float3& _boundsMin, float3& _boundsMax) const;
};
