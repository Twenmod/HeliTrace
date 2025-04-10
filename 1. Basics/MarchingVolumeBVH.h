#pragma once
#include "common.h"
#include "FastNoise.h"

class SkyDome;
class RenderScene;
class FloatTexture;

constexpr uint MAX_VOLUMES = 5;

struct Volume
{
	float3 pos{0.f};
	float3 albedo{1.f};
	float3 emission{500.f, 0.f, 0.f};
	float r{0.f};
	float2 lifeTime = {0.f};
	float rEnd{0.f};
	float faloffStart = 0.5f;
	float emissionDensitySubtraction = 1.f;
	float innerDensityAddition = 1.f;
	float noiseScale = 0.368f;
	float absorption = 0.6f;
	float scattering = 0.9f;
	float g = 0.3f;
	float animationSpeed = 0.02f;
	float lifeValue = 0.f;
	float lifeValueFast = 0.f;
	float sampleR = 0.f;
};

struct VolumeSOA
{
	__m128 x4[MAX_VOLUMES];
	__m128 y4[MAX_VOLUMES];
	__m128 z4[MAX_VOLUMES];
	__m128 r4[MAX_VOLUMES];
	__m128 falloffStart4[MAX_VOLUMES];
	__m128 noiseScale4[MAX_VOLUMES];
	__m128 innerDensity4[MAX_VOLUMES];
};

class MarchingVolumeBVH
{
public:
	MarchingVolumeBVH(float3 _pos, float _r, float3 _albedo, float _absorption, float _scattering,
	                  float _noiseScale = 0.368f,
	                  float _falloffStartPercent = 0.8f,
	                  float _innerDensityAdd = 1.f, float3 _emission = float3(0.f),
	                  float _emissionDensityReduction = 1.f, float _g = 0.3f);
	MarchingVolumeBVH(const Volume* _volumes, uint _amount);
	void Init(RenderScene& _renderScene,
	          const std::function<float3(const float3&, const float3*, float3*, FloatTexture*)>&
	          _calculateLightingFunction);
	~MarchingVolumeBVH();
	//Marches through the volume to calculate light transport
	bool GetContributionThroughVolume(uint _primID, const float3& _rayDir, const float3& _O, uint& _seed,
	                                  float3& _albedo,
	                                  float3& _light) const;

	//Uses beers law to go trough the volume
	float3 GetSimpleContributionThroughVolume(uint _primID, const float3& _rayO, const float3& _rayD,
	                                          uint& _seed) const;
	//float3 GetSimpleContributionThroughVolume4(uint _primID, const float3& _rayO, const float3& _rayD,
	//                                           uint& _seed) const;
	float3 GetSimpleEmissionVolume(uint _primID, const float3& _rayO, const float3& _rayD,
	                               uint& _seed) const;
	//float3 GetSimpleEmissionVolume4(uint _primID, const float3& _rayO, const float3& _rayD,
	//                                uint& _seed) const;
	void Rebuild();
	void Tick(float _deltaTime);
	void Clear();
	tinybvh::BVH bvh;
	Volume* m_volumes;
	std::vector<float4> m_lightVolumes;

	thread_local static float lastT1, lastT2; // store last intersection from volumeintersect to use in getcontribution
	const float BASE_STEP_SIZE{VOLUME_STEP_FULL};
	const float LIGHT_STEP_SIZE{VOLUME_STEP_SIMPLE};
	const float TRANSPARANCY_TRESHOLD{VOLUME_CLIP_TRESHOLD};
	float GetCurrentAnimTime() const { return m_animTimer; }

	void SpawnFromPool(float _lifeTime, const Volume& _volume);

protected:
	float GetDensity(uint _primID, const float3& _P, uint& _seed) const;
	//__m128 GetDensity4(uint _primID, const __m128& _x4, const __m128& _y4, const __m128& _z4, uint& _seed) const;

	// the Henyey-Greenstein phase function
	float Phase(const float _g, const float _cosTheta) const
	{
		float denom = (1 + _g * _g - 2 * _g * _cosTheta) + EPSILON;
		return 1 / (4 * PI) * (1 - _g * _g) / (denom * sqrtf(denom));
	}

	std::function<float3(const float3&, const float3*, float3*, FloatTexture*)> CalculateLighting;
	bool VolumeIntersect(tinybvh::Ray& _ray, const unsigned _primId);
	bool VolumeIsOccluded(const tinybvh::Ray& _ray, const unsigned _primID) const;
	void VolumeAabb(unsigned int _primId, float3& _boundsMin, float3& _boundsMax) const;
	float SampleNoise(const float3& _P, float _scale, uint& _seed) const;
	__m128 SampleNoise4(const __m128& _x, const __m128& _y, const __m128& _z, __m128 _scale, uint& _seed) const;
	VolumeSOA volumeData;
	FastNoise m_noiseGenerator;
	RenderScene* m_renderScene{nullptr};
	float m_animTimer{0};
	uint m_volumeAmount{0};
	const float m_densityCutoff = 0.1f;
	//float* m_noiseTexture;
	//union
	//{
	//	const __m128
	//	const uint m_noiseTextureSize[] = 512;
	//};
};
