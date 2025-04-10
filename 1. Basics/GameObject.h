#pragma once
#include "Mesh.hpp"
#include "tmpl8math.h"

class RenderScene;

class GameObject
{
public:
	GameObject(uint _instance, tinybvh::BLASInstance& _blas, RenderScene& _scene, const std::string& _name);
	GameObject(uint _instance, tinybvh::BLASInstance& _blas, RenderScene& _scene, const std::string& _name,
	           const Animation& _animation);
	virtual ~GameObject() = default;
	virtual void Tick(const float _deltaTime);
	const std::string& GetName() const { return m_name; }
	const float3& GetPosition() const { return m_position; }

	void SetPosition(const float3& _pos)
	{
		m_position = _pos;
	}

	const quat& GetRotation() const { return m_rotation; }

	void SetRotation(const quat& _rot)
	{
		m_rotation = _rot;
	}

	void SetTransform(const mat4& _mat)
	{
		m_position = _mat.GetTranslation();

		m_scale.x = sqrt(_mat[0] * _mat[0] + _mat[4] * _mat[4] + _mat[8] * _mat[8]);
		m_scale.y = sqrt(_mat[1] * _mat[1] + _mat[5] * _mat[5] + _mat[9] * _mat[9]);
		m_scale.z = sqrt(_mat[2] * _mat[2] + _mat[6] * _mat[6] + _mat[10] * _mat[10]);

		m_rotation = mat4ToQuaternion(_mat);
		UpdateTransform();
	}

	virtual void OnShot(float /*_damage*/)
	{
	}

	void UpdateTransform();

	const Animation& GetAnimation() const { return m_animation; }
	float GetAnimTime() const { return m_animTime; }

protected:
	Animation m_animation;
	tinybvh::BLASInstance* m_blas{nullptr};
	RenderScene* m_scene;
	std::string m_name;
	float3 m_position;
	float3 m_scale;
	quat m_rotation;
	float m_animTime{0.f};
	bool m_animated{false};
	uint m_instance;
};
