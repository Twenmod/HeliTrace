#pragma once

struct Cube
{
	__m128 bmin4;
	__m128 bmax4;
	mat4 transform;
	mat4 invTransform;
	int texture = -1;
	int material = 0;

};

class CubeBVH
{
public:
	CubeBVH(mat4 _transform, float3 _halfSize, int _material, int _texture = -1);
	CubeBVH(Cube* _cubes, uint _amount);

	~CubeBVH();
	tinybvh::BVH m_bvh;
	Cube* m_cubes;
protected:
	bool CubeIntersect(tinybvh::Ray& _ray, const unsigned _primID);
	bool CubeIsOccluded(const tinybvh::Ray& _ray, const unsigned _primID);
	void CubeAABB(unsigned int _primID, float3& _boundsMin, float3& _boundsMax);

};

