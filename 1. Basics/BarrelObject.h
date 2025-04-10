#pragma once
#include "GameObject.h"

class MarchingVolumeBVH;

class BarrelObject :
	public GameObject
{
public:
	BarrelObject(MarchingVolumeBVH& _volumeBVH, const GameObject& _base);
	void OnShot(float _damage) override;

private:
	MarchingVolumeBVH* m_volumeBVH;
	float m_health{100.f};
	float m_explosionRadius{20.f};
};
