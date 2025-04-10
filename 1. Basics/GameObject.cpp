#include "precomp.h"
#include "GameObject.h"

GameObject::GameObject(uint _instance, tinybvh::BLASInstance& _blas, RenderScene& _scene, const std::string& _name)
{
	m_instance = _instance;
	m_blas = &_blas;
	m_name = _name;
	m_animated = false;
	m_scene = &_scene;
}

GameObject::GameObject(uint _instance, tinybvh::BLASInstance& _blas, RenderScene& _scene, const std::string& _name,
                       const Animation& _animation)
{
	m_instance = _instance;
	m_blas = &_blas;
	m_name = _name;
	m_animation = _animation;
	m_animated = true;
	m_scene = &_scene;

}


void GameObject::Tick(const float _deltaTime)
{
	if (m_animated)
	{
		m_animTime = m_animTime + _deltaTime;


		float t = (m_animTime) / m_animation.duration;
		t = fmod(t, 1.f - EPSILON);

		t *= static_cast<float>(m_animation.positions.size());
		const int firstKey = clamp(static_cast<int>(floor(t)), 0, static_cast<int>(m_animation.positions.size()));
		const int secondKey = clamp(static_cast<int>(ceil(t)), 0, static_cast<int>(m_animation.positions.size()));


		const float part = t - floor(t);
		m_position = lerp(m_animation.positions[firstKey], m_animation.positions[secondKey], part);
		m_rotation = quat::slerp(m_animation.rotations[firstKey], m_animation.rotations[secondKey], part);

		UpdateTransform();
	}
}

void GameObject::UpdateTransform()
{
	mat4 transform = mat4::Identity();
	transform = transform * mat4::Translate(m_position);
	transform = transform * m_rotation.toMatrix();
	transform = transform * mat4::Scale(m_scale);

	for (int i = 0; i < 16; ++i)
	{
		assert(!isnan(transform[i]));
		m_blas->transform[i] = transform[i];
	}
	m_blas->InvertTransform();
}
