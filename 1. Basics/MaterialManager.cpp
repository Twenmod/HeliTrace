#include "precomp.h"
#include "MaterialManager.h"

MaterialManager::MaterialManager()
= default;

uint MaterialManager::AddMaterial(const float3 _albedo, const float _metallic, const float _transmission,
                                  const float _roughness, const float _IOR)
{
	Material mat;
	mat.hasTextures = false;
	mat.albedo = _albedo;
	mat.emissionStrength = 0.f;
	if (_metallic > _transmission)
	{
		mat.diellectric = false;
		mat.metallic = _metallic;
	}
	else
	{
		mat.diellectric = true;
		mat.transmission = _transmission;
	}
	mat.roughness = _roughness;
	mat.IOR = _IOR;

	m_materials.push_back(mat);
	return static_cast<uint>(m_materials.size()) - 1;
}

uint MaterialManager::AddMaterial(const int _albedoTexture, const int _normalTexture, const int _metalRoughTexture,
                                  int _emissionTexture,
                                  const float _metallic,
                                  const float _transmission, const float _roughness, const float _IOR,
                                  const float _emissionStrength)
{
	Material mat;
	mat.hasTextures = true;
	mat.albedoTexture = _albedoTexture;
	mat.normalTexture = _normalTexture;
	mat.metallicRoughnessTexture = _metalRoughTexture;
	mat.emissionTexture = _emissionTexture;
	mat.emissionStrength = _emissionStrength;
	if (_metallic >= _transmission)
	{
		mat.diellectric = false;
		mat.metallic = _metallic;
	}
	else
	{
		mat.diellectric = true;
		mat.transmission = _transmission;
	}
	mat.roughness = _roughness;
	mat.IOR = _IOR;

	m_materials.push_back(mat);
	return static_cast<uint>(m_materials.size()) - 1;
}
