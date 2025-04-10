#pragma once

struct Material
{
	union
	{
		float3 albedo;

		struct
		{
			int albedoTexture;
			int normalTexture;
			int metallicRoughnessTexture;
			int emissionTexture;
		};
	};

	union
	{
		float metallic;
		float transmission;
	};

	float emissionStrength;
	float roughness;
	float density;
	float IOR;
	bool diellectric;
	bool hasTextures;
};


class MaterialManager
{
public:
	MaterialManager();
	uint AddMaterial(float3 _albedo, float _metallic, float _transmission, float _roughness, float _IOR);
	uint AddMaterial(int _albedoTexture, int _normalTexture, int _metalRoughTexture, int _emissionTexture,
	                 float _metallic, float _transmission,
	                 float _roughness, float _IOR, float _emissionStrength);

	const Material& GetMaterial(uint _id) const
	{
		assert(_id < m_materials.size() || _id > 0);
		if (_id < 0 || _id >= m_materials.size()) _id = 0;
		return m_materials[_id];
	}

	void SetMaterial(const Material& _material, const uint _id)
	{
		m_materials[_id] = _material;
	}

	uint GetMaterialCount() const { return static_cast<uint>(m_materials.size()); }
private:
	std::vector<Material> m_materials;

};
