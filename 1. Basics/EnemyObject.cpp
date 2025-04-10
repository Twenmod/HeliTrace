#include "precomp.h"
#include "EnemyObject.h"

#include "AudioManager.h"
#include "camera.h"
#include "renderer.h"
#include "RenderScene.h"
#include "ScoreManager.h"
#include "SphereBVH.h"

EnemyObject::EnemyObject(Camera& _playerCamera, const mat4& _transform, const GameObject& _base) : GameObject(_base)
{
	SetTransform(_transform);
	m_playerCamera = &_playerCamera;
	m_indicatorMat = m_scene->GetMaterialManager().AddMaterial(float3(0.1f, 0.0, 0.0), 0, 0.f, 0.4f, 1.1f);
	m_attackIndicator = m_scene->AddSphere(m_position + m_rotation.rotateVector(float3(0, 1.95f, 1.15f)), 0.2f,
	                                       static_cast<int>(m_indicatorMat));
	m_attackInterval = Rand(4000.f) + 1000.f;
	m_attackTimer = m_attackInterval;
}

EnemyObject::~EnemyObject()
{
}

void EnemyObject::OnShot(const float _damage)
{
	if (m_health <= 0) return;
	m_health -= _damage;
	if (m_health <= 0)
	{
		//Play sound
		if (RandomFloat() > 0.8f)
			AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/combineDeathSound.wav", false, 0.5f, true,
			                                              m_position,
			                                              0.15f);
		else
			AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/combineDeathSound1.wav", false, 0.5f, true,
			                                              m_position,
			                                              0.15f);
		ScoreManager::GetScoreManager()->AddScore(100.f);
		ScoreManager::GetScoreManager()->MarkEnemyKilled();
		Volume volume;
		volume.pos = m_position + m_rotation.rotateVector(float3(0.f, 1.5f, 0.f));
		volume.r = 0.8f;
		volume.rEnd = 2.f;
		volume.albedo = float3(0.2f, 0.01f, 0.01f);
		volume.emission = float3(0.f);
		volume.emissionDensitySubtraction = 0.1f;
		volume.absorption = 3.f;
		volume.scattering = 3.f;
		volume.noiseScale = 83.237f;
		volume.faloffStart = 0.8f;
		volume.innerDensityAddition = 0.2f;
		volume.animationSpeed = 738.2f;
		m_scene->m_volumeBVH->SpawnFromPool(300.f, volume);

		m_scene->RemoveObject(m_attackIndicator);
		m_scene->RemoveObject(m_instance);
	}
}

void EnemyObject::Tick(const float _deltaTime)
{
	if (m_health <= 0) return;
	//Check line of sight
	float3 dir = GetPosition() - m_playerCamera->camPos;
	float dirDot = dot(dir, GetRotation().rotateVector(float3(0.f, 0.f, 1.f)));
	float dirLength = length(dir);
	if (dirLength > 0) dir = dir / dirLength;
	float t = dirLength - m_lineOfSightOffset * 2;
	if (t < m_maxLineOfSightRange && dirDot < 0)
	{
		tinybvh::Ray ray(
			m_playerCamera->camPos + m_rotation.rotateVector(float3(0.f, 1.6f, 1.1f)) + dir * m_lineOfSightOffset, dir,
			t);
		m_seesPlayer = !m_scene->IsOccluded(ray);
	}
	else m_seesPlayer = false;
	if (m_seesPlayer)
	{
		m_attackTimer -= _deltaTime;

		//Attack visualizer
		float progress = 1.f - (((m_attackTimer / m_attackInterval) + 0.2f) * 0.8f);
		if (progress >= 1.f || progress <= 0.02f)
		{
			m_scene->GetSphereData(m_attackIndicator)->m_spheres[0].r = 0.f;
		}
		else
		{
			Material mat = m_scene->GetMaterialManager().GetMaterial(m_indicatorMat);
			mat.emissionStrength = progress * 50.f;
			m_scene->GetMaterialManager().SetMaterial(mat, m_indicatorMat);

			m_scene->GetSphereData(m_attackIndicator)->m_spheres[0].r = (1.f - progress) * 0.2f;
		}

		if (m_attackTimer <= 0)
		{
			Shoot();
			m_attackTimer = m_attackInterval;
		}
	}
	else
	{
		m_attackTimer = m_attackInterval;
		m_scene->GetSphereData(m_attackIndicator)->m_spheres[0].r = 0.f;
	}

	GameObject::Tick(_deltaTime);
}

void EnemyObject::Shoot()
{
	//Add volume to gun
	Volume volume;
	volume.pos = GetPosition() + GetRotation().rotateVector(float3(0, 2.1f, 1.2f));
	volume.r = 0.6f;
	volume.rEnd = 0.8f;
	volume.albedo = float3(0.1f);
	volume.emission = float3(1500.f, 200.f, 20.f);
	volume.emissionDensitySubtraction = 0.1f;
	volume.absorption = 1.7f;
	volume.scattering = 1.7f;
	volume.noiseScale = 780.237f;
	volume.faloffStart = 0.2f;
	volume.innerDensityAddition = 0.00f;
	volume.animationSpeed = 738.2f;
	m_scene->m_volumeBVH->SpawnFromPool(300.f, volume);

	AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/enemyFire.wav", false, 0.8f, true,
	                                              GetPosition() + GetRotation().rotateVector(
		                                              float3(0.f, 1.7f, 1.2f)), 0.06f);

	Renderer::PlayerTakeDamage(m_damage);



}

void EnemyObject::SetAttackSpeed(const float _randSize, const float _base)
{
	m_attackInterval = Rand(_randSize) + _base;
	m_attackTimer = m_attackInterval;
}
