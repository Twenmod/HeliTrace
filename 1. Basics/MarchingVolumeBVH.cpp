#include "precomp.h"
#include "MarchingVolumeBVH.h"

#include <algorithm>
#include "RenderScene.h"
#include "SkyDome.h"
#include "FastNoise.h"

thread_local float MarchingVolumeBVH::lastT1;
thread_local float MarchingVolumeBVH::lastT2;


MarchingVolumeBVH::MarchingVolumeBVH(float3 _pos, float _r, float3 _albedo, float _absorption, float _scattering,
                                     float _noiseScale,
                                     float _falloffStartPercent, float _innerDensityAdd, float3 _emission,
                                     float _emissionDensityReduction, float _g)
{
	m_volumes = new Volume[1];
	m_volumes[0] = {
		_pos, _albedo, _emission, _r, _falloffStartPercent, _emissionDensityReduction, _innerDensityAdd,
		_noiseScale, _absorption,
		_scattering, _g
	};

	bvh.Build([&](unsigned int _a, float3& _b, float3& _c) { VolumeAabb(_a, _b, _c); }, 1);
	bvh.customIntersect = [&](tinybvh::Ray& _ray, const unsigned _primId) { return VolumeIntersect(_ray, _primId); };
	bvh.customIsOccluded = [&](const tinybvh::Ray& _ray, const unsigned _primId)
	{
		return VolumeIsOccluded(_ray, _primId);
	};
	m_volumeAmount = 1;
}

MarchingVolumeBVH::MarchingVolumeBVH(const Volume* _volumes, const uint _amount)
{
	m_volumes = new Volume[_amount];
	for (uint i = 0; i < _amount; i++)
	{
		m_volumes[i] = _volumes[i];
		volumeData.x4[i] = _mm_set1_ps(_volumes[i].pos.x);
		volumeData.y4[i] = _mm_set1_ps(_volumes[i].pos.y);
		volumeData.z4[i] = _mm_set1_ps(_volumes[i].pos.z);
		volumeData.r4[i] = _mm_set1_ps(_volumes[i].r);
	}

	bvh.Build([&](unsigned int _a, float3& _b, float3& _c) { VolumeAabb(_a, _b, _c); }, _amount);
	bvh.customIntersect = [&](tinybvh::Ray& _ray, const unsigned _primId) { return VolumeIntersect(_ray, _primId); };
	bvh.customIsOccluded = [&](const tinybvh::Ray& _ray, const unsigned _primId)
	{
		return VolumeIsOccluded(_ray, _primId);
	};
	m_volumeAmount = _amount;
}


void MarchingVolumeBVH::Init(RenderScene& _renderScene,
                             const std::function<float3(const float3&, const float3*, float3*, FloatTexture*)>&
                             _calculateLightingFunction)
{
	m_renderScene = &_renderScene;
	CalculateLighting = _calculateLightingFunction;


	////Generate noiseTexture
	//uint totalSize = m_noiseTextureSize * m_noiseTextureSize * m_noiseTextureSize * m_noiseTextureSize;
	//m_noiseTexture = new float[totalSize];
	//for (int i = 0; i < totalSize; ++i)
	//{
	//	int x = i % m_noiseTextureSize;
	//	int y = (i / m_noiseTextureSize) % m_noiseTextureSize;
	//	int z = (i / (m_noiseTextureSize * m_noiseTextureSize)) % m_noiseTextureSize;
	//	int w = (i / (m_noiseTextureSize * m_noiseTextureSize * m_noiseTextureSize)) % m_noiseTextureSize;
	//	m_noiseTexture[i] = m_noiseGenerator.GetSimplex(x, y, z, w);
	//}
}

MarchingVolumeBVH::~MarchingVolumeBVH()
{
	delete[] m_volumes;
}


//TODO: Getting the samples for the raymarching can probably be vectorised


bool MarchingVolumeBVH::GetContributionThroughVolume(uint _primID, const float3& _rayDir, const float3& _rayO,
                                                     uint& _seed,
                                                     float3& _albedo, float3& _light) const
{
	if (_primID > m_volumeAmount) return false;
	if (m_animTimer < m_volumes[_primID].lifeTime.x || m_animTimer > m_volumes[_primID].lifeTime.y) return false;
	float stepSize = BASE_STEP_SIZE * max(m_volumes[_primID].sampleR * 0.5f, 0.5f);
	//Check intersection
	tinybvh::Ray ray(_rayO + _rayDir * (lastT1 + EPSILON), _rayDir, lastT2 - EPSILON * 2);
	m_renderScene->FindNearest(ray);
	lastT2 = std::min(lastT1 + (EPSILON + ray.hit.t), lastT2);

	//Split into UNIFORM steps
	float tSize = lastT2 - lastT1;
	const int steps = static_cast<int>(tSize / stepSize);
	float currT = lastT1;


	auto resultLight = float3(0.f);
	float transmission = 1.f;

	float3 lightValue = clamp(_light, EPSILON, 1.0f);

	float3 backGroundAlbedo = _albedo * lightValue;
	// We have to apply lighting here or fog can bring back data from the background which it shouldnt

	float extinctionCoefficient = m_volumes[_primID].absorption + m_volumes[_primID].scattering;
	for (int step = 0; step < steps; ++step)
	{
		float tSample = currT + (stepSize * RandomFloat(_seed));
		float3 pos = _rayO + _rayDir * tSample;
		float density = GetDensity(_primID, pos, _seed);
		float sampleAttent;
		if (density > EPSILON)
			sampleAttent = exp(-(stepSize + (tSample - currT)) * extinctionCoefficient * density);
		else sampleAttent = 1.0f;
		transmission *= sampleAttent;

		float3 L;
		// beers law already integrated inside calcLighting
		float3 scatteredLight = CalculateLighting(pos, nullptr, &L, nullptr) * m_volumes[_primID].scattering * m_volumes
		[
			_primID].albedo;
		//scatteredLight *= Phase(volumes[_primID].g, dot(_rayDir, L));
		resultLight += scatteredLight * density;
		resultLight += m_volumes[_primID].emission * (m_volumes[_primID].lifeValueFast) * max(
			density - m_volumes[_primID].emissionDensitySubtraction, 0.f);

		//if (transmission <= TRANSPARANCY_TRESHOLD) // Russian roulete
		//{
		//	if (RandomFloat(_seed) < 0.2f)
		//	{
		//		break;
		//	}
		//	else
		//	{
		//		transmission *= 5; // compensate for .2 chance
		//	}
		//}

		currT += BASE_STEP_SIZE;
	}
	if (transmission >= 1.f - m_densityCutoff) return false; // Volume didnt contribute

	_light = _light * transmission + resultLight;
	lightValue = clamp(_light, EPSILON, 1.0f);
	backGroundAlbedo = backGroundAlbedo / lightValue;
	_albedo = lerp(backGroundAlbedo,
	               m_volumes[_primID].albedo, 1 - transmission);
	return true;
}

float3 MarchingVolumeBVH::GetSimpleContributionThroughVolume(uint _primID, const float3& _rayO,
                                                             const float3& _rayD, uint& _seed) const
{
	if (m_animTimer < m_volumes[_primID].lifeTime.x || m_animTimer > m_volumes[_primID].lifeTime.y) return float3(1.f);

	//Split into UNIFORM steps
	float stepSize = LIGHT_STEP_SIZE * max(m_volumes[_primID].sampleR * 0.5f, 0.5f);

	float tSize = lastT2 - lastT1;
	const int steps = min(static_cast<int>(tSize / stepSize), 500);
	float currT = lastT1;

	float density = 0;
	for (int step = 0; step < steps; ++step)
	{
		float3 pos = _rayO + _rayD * (currT + (RandomFloat(_seed) * stepSize));
		density += GetDensity(_primID, pos, _seed);
		currT += stepSize;
	}
	density /= steps;
	float extinctionCoefficient = m_volumes[_primID].absorption + m_volumes[_primID].scattering;
	float beer;
	if (density > EPSILON)
		beer = exp(-tSize * extinctionCoefficient * density);
	else beer = 1.f;
	return float3(beer) + m_volumes[_primID].emission * (m_volumes[_primID].lifeValueFast) * max(
		density - m_volumes[_primID].emissionDensitySubtraction,
		0.f);
}

//float3 MarchingVolumeBVH::GetSimpleContributionThroughVolume4(uint _primID, const float3& _rayO, const float3& _rayD,
//                                                              uint& _seed) const
//{
//
//	//Split into UNIFORM steps
//	float tSize = lastT2 - lastT1;
//	const int steps = static_cast<int>(tSize / LIGHT_STEP_SIZE);
//	float currT = lastT1;
//
//	float density = 0;
//	__m128 rayOX4 = _mm_set1_ps(_rayO.x);
//	__m128 rayOY4 = _mm_set1_ps(_rayO.y);
//	__m128 rayOZ4 = _mm_set1_ps(_rayO.z);
//	__m128 rayDX4 = _mm_set1_ps(_rayD.x);
//	__m128 rayDY4 = _mm_set1_ps(_rayD.y);
//	__m128 rayDZ4 = _mm_set1_ps(_rayD.z);
//	for (int step = 0; step < steps / 4; ++step)
//	{
//		__m128 currT4 = _mm_set1_ps(currT);
//		__m128 pX4 = _mm_add_ps(rayOX4, _mm_mul_ps(rayDX4, currT4));
//		__m128 pY4 = _mm_add_ps(rayOY4, _mm_mul_ps(rayDY4, currT4));
//		__m128 pZ4 = _mm_add_ps(rayOZ4, _mm_mul_ps(rayDZ4, currT4));
//		union
//		{
//			float dens[4];
//			__m128 dens4;
//		};
//		dens4 = GetDensity4(_primID, pX4, pY4, pZ4, _seed);
//		density += dens[0] + dens[1] + dens[2] + dens[3];
//	}
//	density /= steps;
//	float extinctionCoefficient = volumes[_primID].absorption + volumes[_primID].scattering;
//	float beer;
//	if (density > EPSILON)
//		beer = exp(-tSize * extinctionCoefficient * density);
//	else beer = 1.f;
//	return float3(beer) + volumes[_primID].emission * (volumes[_primID].lifeValueFast) * max(
//		density - volumes[_primID].emissionDensitySubtraction,
//		0.f);
//}

float3 MarchingVolumeBVH::GetSimpleEmissionVolume(uint _primID, const float3& _rayO, const float3& _rayD,
                                                  uint& _seed) const
{
	if (m_animTimer < m_volumes[_primID].lifeTime.x || m_animTimer > m_volumes[_primID].lifeTime.y) return float3(0.f);
	float stepSize = LIGHT_STEP_SIZE * max(m_volumes[_primID].sampleR * 0.5f, 0.5f);

	//Split into UNIFORM steps
	float tSize = lastT2 - lastT1;
	const int steps = static_cast<int>(tSize / stepSize);
	float currT = lastT1;

	float density = 0;
	for (int step = 0; step < steps; ++step)
	{
		float3 pos = _rayO + _rayD * (currT + (RandomFloat(_seed) * stepSize));
		density += GetDensity(_primID, pos, _seed);
		currT += LIGHT_STEP_SIZE;
	}
	density /= steps;
	float extinctionCoefficient = m_volumes[_primID].absorption + m_volumes[_primID].scattering;
	float beer;
	if (density > EPSILON)
		beer = exp(-tSize * extinctionCoefficient * density);
	else beer = 1.f;
	return float3(1.f - beer) * m_volumes[_primID].emission * (m_volumes[_primID].lifeValueFast) *
		max(
			density - m_volumes[_primID].emissionDensitySubtraction,
			0.f);
}

//float3 MarchingVolumeBVH::GetSimpleEmissionVolume4(uint _primID, const float3& _rayO, const float3& _rayD,
//                                                   uint& _seed) const
//{
//	//Split into UNIFORM steps
//	float tSize = lastT2 - lastT1;
//	const int steps = static_cast<int>(tSize / LIGHT_STEP_SIZE);
//	float currT = lastT1;
//
//	float density = 0;
//	__m128 rayOX4 = _mm_set1_ps(_rayO.x);
//	__m128 rayOY4 = _mm_set1_ps(_rayO.y);
//	__m128 rayOZ4 = _mm_set1_ps(_rayO.z);
//	__m128 rayDX4 = _mm_set1_ps(_rayD.x);
//	__m128 rayDY4 = _mm_set1_ps(_rayD.y);
//	__m128 rayDZ4 = _mm_set1_ps(_rayD.z);
//	for (int step = 0; step < steps / 4; ++step)
//	{
//		__m128 currT4 = _mm_set1_ps(currT);
//		__m128 pX4 = _mm_add_ps(rayOX4, _mm_mul_ps(rayDX4, currT4));
//		__m128 pY4 = _mm_add_ps(rayOY4, _mm_mul_ps(rayDY4, currT4));
//		__m128 pZ4 = _mm_add_ps(rayOZ4, _mm_mul_ps(rayDZ4, currT4));
//		union
//		{
//			float dens[4];
//			__m128 dens4;
//		};
//		dens4 = GetDensity4(_primID, pX4, pY4, pZ4, _seed);
//		density += dens[0] + dens[1] + dens[2] + dens[3];
//	}
//	//Add last remaining steps
//	int remainder = steps % 4;
//
//	__m128 currT4 = _mm_set1_ps(currT);
//	__m128 pX4 = _mm_add_ps(rayOX4, _mm_mul_ps(rayDX4, currT4));
//	__m128 pY4 = _mm_add_ps(rayOY4, _mm_mul_ps(rayDY4, currT4));
//	__m128 pZ4 = _mm_add_ps(rayOZ4, _mm_mul_ps(rayDZ4, currT4));
//	union
//	{
//		float dens[4];
//		__m128 dens4;
//	};
//	dens4 = GetDensity4(_primID, pX4, pY4, pZ4, _seed);
//	for (int i = 0; i < remainder; ++i)
//	{
//		density += dens[i];
//	}
//
//	density /= steps;
//	float extinctionCoefficient = volumes[_primID].absorption + volumes[_primID].scattering;
//	float beer;
//	if (density > EPSILON)
//		beer = exp(-tSize * extinctionCoefficient * density);
//	else beer = 1.f;
//	return float3(1.f - beer) * volumes[_primID].emission * (volumes[_primID].lifeValueFast) *
//		max(
//			density - volumes[_primID].emissionDensitySubtraction,
//			0.f);
//}

void MarchingVolumeBVH::Rebuild()
{
	if (bvh.refittable) bvh.Refit();
	else bvh.Build([&](unsigned int _a, float3& _b, float3& _c) { VolumeAabb(_a, _b, _c); }, m_volumeAmount);
}


void MarchingVolumeBVH::Tick(float _deltaTime)
{
	m_animTimer += _deltaTime;

	//Count how many volumes are light sources
	m_lightVolumes.clear();
	for (uint i = 0; i < m_volumeAmount; ++i)
	{
		Volume& volume = m_volumes[i];
		if (m_animTimer < volume.lifeTime.x || m_animTimer > volume.lifeTime.y)
		{
			volume.lifeValue = 0;
			continue;
		}


		const float midPoint = 0.025f;
		float lifeTime = volume.lifeTime.y - volume.lifeTime.x;
		float mid = volume.lifeTime.x + lifeTime * midPoint;

		if (m_animTimer > mid)
		{
			volume.lifeValue = clamp(1.f - (m_animTimer - mid) / (volume.lifeTime.y - mid), 0.f, 1.f);
			volume.lifeValueFast = clamp(
				1.f - (m_animTimer - mid) / ((volume.lifeTime.y - mid) - (volume.lifeTime.y - mid) *
					0.6f), 0.f, 1.f);
			volume.sampleR = lerp(volume.r, volume.rEnd, 1.f - volume.lifeValue);
		}
		else
		{
			volume.lifeValue = (m_animTimer - volume.lifeTime.x) / (mid - volume.lifeTime.x);
			volume.lifeValueFast = volume.lifeValue;
			volume.sampleR = volume.r * volume.lifeValue;
		}
		float r = volume.r * volume.lifeValue;
		if (length2(volume.emission) > 0.f && r > EPSILON) m_lightVolumes.push_back(float4(volume.pos, r));
	}
}

void MarchingVolumeBVH::Clear()
{
	for (uint i = 0; i < m_volumeAmount; ++i)
	{
		m_volumes->lifeTime = float2(0.f);
	}
}

void MarchingVolumeBVH::SpawnFromPool(const float _lifeTime, const Volume& _volume)
{
	for (uint i = 0; i < m_volumeAmount; ++i)
	{
		if (m_animTimer < m_volumes[i].lifeTime.x || m_animTimer > m_volumes[i].lifeTime.y)
		{
			m_volumes[i] = _volume;
			m_volumes[i].lifeTime = float2(
				GetCurrentAnimTime(),
				GetCurrentAnimTime() + _lifeTime);
			return;
		}
	}
}

float MarchingVolumeBVH::GetDensity(uint _primID, const float3& _P, uint& _seed) const
{
	if (m_animTimer < m_volumes[_primID].lifeTime.x || m_animTimer > m_volumes[_primID].lifeTime.y) return 0.f;


	float dist2 = length2(m_volumes[_primID].pos - _P);

	float r = m_volumes[_primID].sampleR;
	float r2 = r * r;


	float falloffStart = r2 * m_volumes[_primID].faloffStart;
	float falloff = 1 - clamp((dist2 - falloffStart) / (r2 - falloffStart), 0.f, 1.f);
	float density = SampleNoise(_P + float3(100.f), m_volumes[_primID].noiseScale, _seed) * 0.5f;
	density += (1 - (dist2 / r2)) * m_volumes[_primID].innerDensityAddition;
	density *= m_volumes[_primID].lifeValue;
	return max(density * falloff, 0.f);
}

//__m128 MarchingVolumeBVH::GetDensity4(uint _primID, const __m128& _x4, const __m128& _y4, const __m128& _z4,
//                                      uint& _seed) const
//{
//	if (m_animTimer < volumes[_primID].lifeTime.x || m_animTimer > volumes[_primID].lifeTime.y) return _mm_setzero_ps();
//
//	__m128 xDist = _mm_sub_ps(volumeData.x4[_primID], _x4);
//	__m128 yDist = _mm_sub_ps(volumeData.y4[_primID], _y4);
//	__m128 zDist = _mm_sub_ps(volumeData.z4[_primID], _z4);
//	__m128 dist2 = dot(xDist, yDist, zDist, xDist, yDist, zDist);
//	__m128 r = _mm_set1_ps(volumes[_primID].r);
//	__m128 r2 = _mm_mul_ps(r, r);
//
//	__m128 falloffStart = _mm_mul_ps(r2, volumeData.falloffStart4[_primID]);
//	__m128 distMinFall = _mm_sub_ps(dist2, falloffStart);
//	__m128 r2MinFall = _mm_sub_ps(r2, falloffStart);
//	__m128 unclampedFalloff = _mm_div_ps(distMinFall, r2MinFall);
//	__m128 clampedFalloff = _mm_max_ps(_mm_min_ps(unclampedFalloff, _mm_setzero_ps()), _mm_set1_ps(1.f));
//	__m128 falloff = _mm_sub_ps(_mm_set1_ps(1.f), clampedFalloff);
//	__m128 density = SampleNoise4(_x4, _y4, _z4, volumeData.noiseScale4[_primID], _seed);
//	density = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.f), _mm_div_ps(dist2, r2)), volumeData.innerDensity4[_primID]);
//
//
//	return _mm_max_ps(_mm_mul_ps(density, falloff), _mm_setzero_ps());
//}

bool MarchingVolumeBVH::VolumeIntersect(tinybvh::Ray& _ray, const unsigned _primId)
{
	if (m_animTimer < m_volumes[_primId].lifeTime.x || m_animTimer > m_volumes[_primId].lifeTime.y) return false;

	float rayLength = length(_ray.D);
	_ray.D /= rayLength;
	_ray.hit.t *= rayLength;

	const float3 oc = _ray.O - m_volumes[_primId].pos;
	const float b = dot(oc, _ray.D);
	const float r = m_volumes[_primId].sampleR;
	const float c = dot(oc, oc) - r * r;
	float t, d = b * b - c;
	if (d <= 0) return false;
	d = sqrtf(d);
	const float t1 = -b - d;
	const float t2 = -b + d;
	if (t1 > 0)
	{
		t = t1;
	}
	else
	{
		t = t2;
		if (t < 0) return false; // The sphere is behind the ray origin
	}
	// Check if this intersection is closer than the current hit
	const bool hit = (t1 < _ray.hit.t || t2 < _ray.hit.t) && t > 0;
	if (hit)
	{
		lastT1 = max(t1, 0.f); // clamp t1 to where the ray started
		lastT2 = t2;
		_ray.hit.t = min(t1, t2);
		_ray.hit.prim = _primId;
	}

	return hit;
}

bool MarchingVolumeBVH::VolumeIsOccluded(const tinybvh::Ray& _ray, const unsigned _primID) const
{
	if (m_animTimer < m_volumes[_primID].lifeTime.x || m_animTimer > m_volumes[_primID].lifeTime.y) return false;

	float rayLength = length(_ray.D);
	const float3 D = _ray.D / rayLength;

	const float3 oc = _ray.O - m_volumes[_primID].pos;
	const float b = dot(oc, D);
	const float r = m_volumes[_primID].r;
	const float c = dot(oc, oc) - r * r;
	float t, d = b * b - c;
	if (d <= 0) return false;
	d = sqrtf(d);
	const float t1 = -b - d;
	const float t2 = -b + d;
	if (t1 > 0)
	{
		t = t1;
	}
	else
	{
		t = t2;
		if (t < 0) return false; // The sphere is behind the ray origin
	}
	const float rayT = _ray.hit.t * rayLength;


	return t < rayT && t > 0;
}

void MarchingVolumeBVH::VolumeAabb(unsigned int _primId, float3& _boundsMin, float3& _boundsMax) const
{
	if (m_animTimer < m_volumes[_primId].lifeTime.x || m_animTimer > m_volumes[_primId].lifeTime.y)
	{
		_boundsMin = m_volumes[_primId].pos;
		_boundsMax = m_volumes[_primId].pos;
	}
	else
	{
		_boundsMin = m_volumes[_primId].pos - float3(m_volumes[_primId].r);
		_boundsMax = m_volumes[_primId].pos + float3(m_volumes[_primId].r);
	}
}

float MarchingVolumeBVH::SampleNoise(const float3& _P, const float _scale, uint& /*_seed*/) const
{
	float3 P = float3(_P.x * _scale, _P.y * _scale, _P.z * _scale);
	float value = m_noiseGenerator.GetSimplex(P.x, P.y, P.z, m_animTimer * 0.01f);
	return value;
}

//__m128 MarchingVolumeBVH::SampleNoise4(const __m128& _x, const __m128& _y, const __m128& _z, const __m128 _scale,
//                                       uint& _seed) const
//{
//	union
//	{
//		__m128 x4;
//		float x[4];
//	};
//	x4 = _mm_mul_ps(_x, _scale);
//	union
//	{
//		__m128 y4;
//		float y[4];
//	};
//	y4 = _mm_mul_ps(_y, _scale);
//	union
//	{
//		__m128 z4;
//		float z[4];
//	};
//	z4 = _mm_mul_ps(_z, _scale);
//
//	union
//	{
//		__m128 noise4;
//		float noise[4];
//	};
//
//	__m128i ix4 = _mm_cvttps_epi32(x4);
//	__m128i iy4 = _mm_cvttps_epi32(y4);
//	__m128i iz4 = _mm_cvttps_epi32(z4);
//
//	__m128 iyi = _mm_mul_epi32(iy4,)
//
//	for (int i = 0; i < 4; ++i)
//	{
//		noise[i] = m_noiseTexture[]
//	}
//	return result4;
//}
