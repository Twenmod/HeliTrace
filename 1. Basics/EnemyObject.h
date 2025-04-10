#pragma once
#include "GameObject.h"

class SphereBVH;

namespace Tmpl8
{
	class Camera;
}

class EnemyObject : public GameObject
{
public:
	EnemyObject(Camera& _playerCamera, const mat4& _transform, const GameObject& _base);
	~EnemyObject() override;
	void OnShot(float _damage) override;
	void Tick(const float _deltaTime) override;
	void Shoot();
	void SetAttackSpeed(float _randSize, float _base);
	void SetMaxRange(const float _maxRange) { m_maxLineOfSightRange = _maxRange; }
private:
	int m_attackIndicator;
	Camera* m_playerCamera;
	uint m_indicatorMat;
	float m_health{100.f};
	bool m_seesPlayer{false};
	float m_attackInterval{4000.f};
	float m_attackTimer{m_attackInterval};
	float m_damage{1.f};
	float m_lineOfSightOffset = 1.5f;
	float m_maxLineOfSightRange = 17.f;
};
