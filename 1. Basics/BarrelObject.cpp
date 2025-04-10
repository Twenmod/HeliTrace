#include "precomp.h"
#include "BarrelObject.h"

#include "AudioManager.h"
#include "RenderScene.h"
#include "ScoreManager.h"


BarrelObject::BarrelObject(MarchingVolumeBVH& _volumeBVH, const GameObject& _base) : GameObject(_base)
{
	m_volumeBVH = &_volumeBVH;
}

void BarrelObject::OnShot(float _damage)
{
	m_health -= _damage;
	if (m_health <= 0)
	{
		//Play sound
		AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/explosion.wav", false, 12.f, true, m_position,
		                                              0.10f);

		//Add volume
		Volume volume;
		volume.pos = m_position;
		volume.r = 3.f;
		volume.rEnd = 4.f;

		volume.albedo = float3(0.1f);
		volume.emission = float3(800.f, 200.f, 20.f);
		volume.emissionDensitySubtraction = 0.2f;
		volume.absorption = 1.4f;
		volume.scattering = 1.4f;
		volume.noiseScale = 63.237f;
		volume.faloffStart = 0.6f;
		volume.innerDensityAddition = 0.2f;
		volume.animationSpeed = 238.2f;
		m_volumeBVH->SpawnFromPool(4500.f, volume);
		ScoreManager::GetScoreManager()->AddScore(200.f);
		GameObject** objects = m_scene->GetGameObjects();
		for (uint i = 0; i <= m_scene->GetObjectAmount(); i++)
		{
			GameObject* object = objects[i];
			if (object != nullptr && object != this)
			{
				if (length2(object->GetPosition() - GetPosition()) < m_explosionRadius)
				{
					ScoreManager::GetScoreManager()->AddScore(50.f);
					object->OnShot(100.f);
				}
			}
		}


		m_scene->RemoveObject(m_instance);
	}
}
