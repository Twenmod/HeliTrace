#include "precomp.h"
#include "camera.h"
#include "scene.h"
#include "renderer.h"

#include "AudioManager.h"
#include "EnemyObject.h"
#include "Model.hpp"
#include "TextureManager.h"
#include "RenderScene.h"
#include "floattexture.h"
#include "GameObject.h"
#include "MarchingVolumeBVH.h"
#include "miniaudio.h"
#include "ScoreManager.h"
#include "SphereBVH.h"
#include "SkyDome.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------


thread_local int2 rayScreenPos;
uint blueNoiseSamples[RENDER_WIDTH * RENDER_HEIGHT] = {0};
thread_local uint seed = 0U;
thread_local __m128i seed4;

float Renderer::m_heliHealth = 500;
bool Renderer::m_playerDied = false;

void Renderer::Init()
{
	ScoreManager::GetScoreManager()->ResetScore();
	m_playerDied = false;

	m_renderScene = new RenderScene(m_camera);
	//Set camera
	m_camera.camPos = float3(7, 7, 7);
	m_camera.camTarget = float3(8);
	m_camera.CalcView();

	AudioManager::GetAudioManager().SetListener(m_camera);

	//Add volume
	Volume volData;
	volData.pos = float3(2, 3, 2);
	volData.r = 4.f;
	volData.albedo = float3(0.2f);
	volData.emission = float3(800.f, 200.f, 20.f);
	//float3(1500.f, 800.f, 100.f);
	volData.emissionDensitySubtraction = 0.2f;
	volData.absorption = 1.4f;
	volData.scattering = 1.4f;
	volData.noiseScale = 63.237f;
	volData.faloffStart = 0.6f;
	volData.innerDensityAddition = 0.2f;
	volData.animationSpeed = 83.2f;
	volData.lifeTime = float2(0, 0);

	Volume volDatas[MAX_SIMUL_VOLUMES];
	for (int i = 0; i < MAX_SIMUL_VOLUMES; ++i)
	{
		volDatas[i] = volData;
	}

	auto volume = new MarchingVolumeBVH(&volDatas[0], MAX_SIMUL_VOLUMES);
	volume->Init(*m_renderScene, [&](const float3& I, const float3* N, float3* outDir, FloatTexture* _debugScreen)
	{
		return CalculateLighting(I, N, outDir, _debugScreen);
	});
	m_renderScene->m_volumeBVH = volume;


	auto level = new Model();
	level->Init("../assets/city/level.gltf", m_renderScene->GetTextureManager(), m_renderScene->GetMaterialManager(),
	            *m_renderScene);
	m_renderScene->AddModel(*level);
	//Load animation
	//Model anim = Model();
	//anim.Init("../assets/animated.glb", renderScene.GetTextureManager(), renderScene.GetMaterialManager(), renderScene);
	//renderScene.AddModel(anim);

	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise1.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise2.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise3.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise4.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise5.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise6.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise7.png", true));
	m_noiseTextures.push_back(new FloatTexture("../assets/BlueNoise8.png", true));

	//Load skydome
	m_skyDome = new SkyDome(new FloatTexture("../assets/skydomes/dussel.hdr"));

	{
		////Loads of balls
		//int matStart = renderScene.GetMaterialManager().AddMaterial(float3(1.f), 0.f, 0.f, 0.0f, 1.f);
		//renderScene.GetMaterialManager().AddMaterial(float3(1.f, 0.f, 0.f), 0.f, 0.f, 0.f, 1.f);
		//renderScene.GetMaterialManager().AddMaterial(float3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f);

		////Add Loads of balls
		//const uint amount = 10;
		//const float range = 3;
		//uint seed = InitSeed(69);
		//Sphere spheres[amount];
		//for (int i = 0; i < amount; i++)
		//{
		//	spheres[i].pos = RandomUnitVector(seed) * RandomFloat(seed) * range;
		//	spheres[i].r = (RandomFloat(seed) + 0.2f) * 1;
		//	spheres[i].material = static_cast<int>(matStart + floor(RandomFloat(seed) * 2));
		//}
		//group1Index = renderScene.AddSphere(spheres, amount);

		////Add Loads of balls
		//for (int i = 0; i < amount; i++)
		//{
		//	spheres[i].pos = RandomUnitVector(seed) * RandomFloat(seed) * range;
		//	spheres[i].r = (RandomFloat(seed) + 0.2f) * 1;
		//	spheres[i].material = static_cast<int>(matStart + floor(RandomFloat(seed) * 2));
		//}
		//group2Index = renderScene.AddSphere(spheres, amount);
	}
	{
		//// Colored lights
		//renderScene.AddLight({float3(-3.f, 0.5f, 0.f), float3{20.f, 0.f, 0.f}, float3(1.f)});
		//renderScene.AddLight({float3(0.f, 0.5f, 3.f), float3{0.f, 20.f, 0.f}, float3(1.0f)});
		//renderScene.AddLight({float3(3.f, 0.5f, 0.f), float3{0.f, 0.f, 20.f}, float3(1.0f)});
	}
	{
		// Bright light
		//renderScene.AddLight({ float3(0.f,2.f,-1.f), float3{50.f,50.f,50.f}, float3(0.3f) });
	}
	//int spotTexture = renderScene.GetTextureManager().LoadTexture("../assets/texture.jpg", true);
	//renderScene.AddSpotLight(float3(0.f, 2.f, 0.f), normalize(float3(0.f, -1.f, 0.f)), spotTexture, 3.f, 500.f, 60.f,90.f);
	//renderScene.AddSpotLight(float3(0.f,0.5f,0.f), float3(0.f,0.f,-1.f), float3(50.f,0.f,0.f), 40.f,50.f);
	//renderScene.AddDirLight({normalize(float3(1, -0.8, -1)), float3(2.f)});

	m_renderScene->BuildTLAS();
	m_renderScene->SetupLightSOA();

	// create fp32 rgb pixel buffer to render to
	m_accumulatorAlbedo = static_cast<float4*>(MALLOC64(RENDER_WIDTH * RENDER_HEIGHT * 16));
	m_accumulatorLight = static_cast<float4*>(MALLOC64(RENDER_WIDTH * RENDER_HEIGHT * 16));
	if (m_accumulatorAlbedo == nullptr || m_accumulatorLight == nullptr)
	{
		DebugBreak();
		abort();
	}
	memset(m_accumulatorAlbedo, 0, RENDER_WIDTH * RENDER_HEIGHT * 16);
	memset(m_accumulatorLight, 0, RENDER_WIDTH * RENDER_HEIGHT * 16);


	//Init imgui flags
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;


	//Set seed
#pragma omp parallel for schedule(dynamic)  // NOLINT(clang-diagnostic-source-uses-openmp)
	for (int y = 0; y < RENDER_HEIGHT; y++)
	{
		__m128i seed = _mm_set_epi32(y * 4, y * 4 + 1, y * 4 + 2, y * 4 + 3);
		seed4 = InitSeed(seed);
	}
	SetDifficulty(Difficulty::Normal);

	m_renderScene->Tick(0.5f);
	m_cursorLockMode = GLFW_CURSOR_NORMAL;
}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------


void Renderer::CalculateContributionThroughVolumes(float4* _outPos, float4* _outNormal, TraceOut& _out,
                                                   const tinybvh::Ray& _tinyRay, int _bounces)
{
	if (_bounces <= 0) return;
	//Calculate volumes
	if (m_renderScene->m_volumeBVH != nullptr)
	{
		tinybvh::Ray volumeRay = _tinyRay;
		m_renderScene->m_volumeBVH->bvh.Intersect(volumeRay);
		float oldT = min(_tinyRay.hit.t, 1.0e20f);
		if (volumeRay.hit.t + EPSILON < oldT)
		{
			bool contributed = m_renderScene->m_volumeBVH->GetContributionThroughVolume(
				volumeRay.hit.prim, normalize(volumeRay.D),
				volumeRay.O, seed,
				_out.albedo, _out.lighting);

			if (contributed)
			{
				float3 I = volumeRay.O + volumeRay.D * volumeRay.hit.t;
				if (_outPos != nullptr)
				{
					_outPos->x = I.x;
					_outPos->y = I.y;
					_outPos->z = I.z;
					_outPos->w = 2.f; // accumulate but dont reproject
				}
				if (_outNormal != nullptr)
				{
					float3 normal = normalize(I - m_renderScene->m_volumeBVH->m_volumes[volumeRay.hit.prim].pos);
					_outNormal->x = normal.x;
					_outNormal->y = normal.y;
					_outNormal->z = normal.z;
					_outNormal->w = volumeRay.hit.t;
				}
			}
			CalculateContributionThroughVolumes(nullptr, nullptr, _out,
			                                    tinybvh::Ray(
				                                    volumeRay.O + (m_renderScene->m_volumeBVH->lastT2 + EPSILON * 2.f) *
				                                    volumeRay.D, volumeRay.D), _bounces - 1); // recurse
		} // else no contribution
	}
}

void Renderer::CalculateRefractive(uint _bounces, float4* _outNormal, float4* _outPos, FloatTexture* _debugScreen,
                                   bool _primaryDebugRay, TraceOut& _out, tinybvh::Ray _tinyRay, float3& _albedo,
                                   float3 _i,
                                   bool _frontFace, float3 _normal, const Material& _material, float _transmission,
                                   float _roughness)
{
	float roughness = _roughness * 0.1f;
	// * 0.1 because blender does the roughness significantly less

	float IOR = _material.IOR;

	float ri = _frontFace ? (1.0f / IOR) : IOR;

	float3 rayDir = normalize(_tinyRay.D);


	float3 direction;

	float cosTheta = min(dot(-rayDir, _normal), 1.0f);
	float reflectivity = reflectance(cosTheta, ri);
#ifndef DISABLE_NON_REPROJECTABLE
	if (reflectivity > SampleBlueNoise())
	{
		direction = reflect(rayDir, normal);
	}
	else
	{
		direction = refract(rayDir, normal, ri);
		if (dot(direction, direction) < 0.5f) direction = reflect(rayDir, normal); // no refraction possible
	}

	float3 rngDirection = direction;

	if (roughness > 0.f)
	{
		rngDirection = normalize(direction + BlueRandomUnitVector() * roughness);
	}

	float4 refractNormal;
	float4 refractPos;
	TraceOut refractOut = Trace(Ray(I + rngDirection * EPSILON, rngDirection), _bounces - 1, &refractNormal,
		&refractPos, _debugScreen, _primaryDebugRay);
#else

	direction = refract(rayDir, _normal, ri);
	float4 refractNormal = float4(0.f);
	float4 refractPos = float4(0.f);
	TraceOut refractOut;
	if (dot(direction, direction) < 0.5f)
	{
		reflectivity = 1.0f; // no refraction possible
		refractPos.w = 1.f; // no reprojection
	}
	else
	{
		float3 rngDirection = direction;
		if (roughness > 0.f)
		{
			rngDirection = normalize(direction + RandomUnitVector(seed) * roughness);
		}
		refractOut = Trace(Ray(_i + rngDirection * EPSILON, rngDirection), _bounces - 1, &refractNormal,
		                   &refractPos, _debugScreen, _primaryDebugRay);
	}
	if (reflectivity > 0.f)
	{
		direction = reflect(rayDir, _normal);
		float3 rngDirection = direction;
		if (roughness > 0.f)
		{
			rngDirection = normalize(direction + RandomUnitVector(seed) * roughness);
		}
		TraceOut reflectOut;
		reflectOut = Trace(Ray(_i + rngDirection * EPSILON, rngDirection), _bounces - 1, nullptr,
		                   nullptr, _debugScreen, _primaryDebugRay);
		refractOut.albedo = lerp(refractOut.albedo * _material.albedo, reflectOut.albedo * _material.albedo,
		                         reflectivity);
		refractOut.lighting = lerp(refractOut.lighting, reflectOut.lighting, reflectivity);
	}

#endif

	if (_transmission == 1.f && _material.density == 0.f)
	// Only reproject the refracted surface if the dielectric itself doesnt contribute to the color
	{
		if (_outNormal != nullptr)
		{
			_outNormal->x = refractNormal.x;
			_outNormal->y = refractNormal.y;
			_outNormal->z = refractNormal.z;
			_outNormal->w = refractNormal.w;
		}
		if (_outPos != nullptr)
		{
			_outPos->x = refractPos.x;
			_outPos->y = refractPos.y;
			_outPos->z = refractPos.z;
			_outPos->w = refractPos.w;
		}
	}
	else
	{
		if (_outNormal != nullptr)
		{
			_outNormal->x = _normal.x;
			_outNormal->y = _normal.y;
			_outNormal->z = _normal.z;
			//normal A/w value contains depth info
			_outNormal->w = _tinyRay.hit.t;
		}
		if (_outPos != nullptr)
		{
			_outPos->x = _i.x;
			_outPos->y = _i.y;
			_outPos->z = _i.z;
			_outPos->w = 2.f; // Accumulate but dont reproject
		}
	}

	//Beers law

	float3 transmittedAlbedo;
	if (_transmission < 1.f)
	{
		transmittedAlbedo = lerp(_albedo, refractOut.albedo, _transmission);
	}
	else transmittedAlbedo = refractOut.albedo;

	if (_frontFace)
	{
		if (_material.density > EPSILON && refractOut.length > EPSILON)
		{
			float T = exp(-refractOut.length * _material.density);
			_albedo = lerp(_albedo, transmittedAlbedo, T);
		}
		else _albedo = transmittedAlbedo;
	}
	else
	{
		_albedo = transmittedAlbedo;
	}

	_out.lighting = refractOut.lighting;
}

void Renderer::CalculateMettalic(const Tmpl8::Ray& _ray, uint _bounces, float4* _outNormal, float4* _outPos,
                                 FloatTexture* _debugScreen, bool _primaryDebugRay, TraceOut& _out,
                                 tinybvh::Ray _tinyRay,
                                 float3& _albedo, float3 _i, float3 _normal, float _roughness, float _reflectivity)
{
	float3 reflectDirection = reflect(_ray.D, _normal);
	if (_roughness > 0)
	{
		reflectDirection = normalize(reflectDirection + BlueRandomUnitVector() * _roughness);
		if (dot(reflectDirection, _normal) < 0)
		{
			reflectDirection = normalize(reflect(_ray.D, _normal));
		}
	}
	if (_debugScreen == nullptr || _primaryDebugRay)
	{
		float4 reflectNormal;
		float4 reflectPos;
		TraceOut reflectOut = Trace(Ray(_i + reflectDirection * EPSILON, reflectDirection), _bounces - 1,
		                            &reflectNormal, &reflectPos, _debugScreen, _primaryDebugRay);

		if (_roughness == 0.f && _reflectivity > 0.9f && length2(_albedo) == 3.f)
		// Don't reproject if reflection isn't clear
		{
			if (_outNormal != nullptr)
			{
				_outNormal->x = reflectNormal.x;
				_outNormal->y = reflectNormal.y;
				_outNormal->z = reflectNormal.z;
				_outNormal->w = reflectNormal.w;
			}
			if (_outPos != nullptr)
			{
				_outPos->x = reflectPos.x;
				_outPos->y = reflectPos.y;
				_outPos->z = reflectPos.z;
				_outPos->w = reflectPos.w;
			}
		}
		else
		{
			if (_outNormal != nullptr)
			{
				_outNormal->x = _normal.x;
				_outNormal->y = _normal.y;
				_outNormal->z = _normal.z;
				//normal A/w value contains depth info
				_outNormal->w = _tinyRay.hit.t;
			}
			if (_outPos != nullptr)
			{
				_outPos->x = _i.x;
				_outPos->y = _i.y;
				_outPos->z = _i.z;
				_outPos->w = 0.f; // Reproject
			}
		}
		_out.lighting = _reflectivity * (reflectOut.lighting * reflectOut.albedo);
	}
}

TraceOut Renderer::Trace(const Ray& _ray, uint _bounces, float4* _outNormal, float4* _outPos,
                         FloatTexture* _debugScreen, bool _primaryDebugRay)
{
	TraceOut out;
	out.albedo = float3(0);
	out.lighting = float3(0);
	if (_bounces == 0)
	{
		out.length = 1.0e30f;
		if (_outPos != nullptr) _outPos->w = 1.f; // Do not reproject
		return out;
	}

	const bool isDebug = _debugScreen != nullptr;

	if (_primaryDebugRay && m_debugSettings.breakNext)
	{
		m_debugSettings.breakNext = false;
		DebugBreak();
	}

	auto tinyRay = tinybvh::Ray(_ray.O, _ray.D);
	m_renderScene->FindNearest(tinyRay);
	if (tinyRay.hit.t >= 1.0E20f)
	{
		// no hit return background
		if (isDebug)
		{
			if (_primaryDebugRay)
			{
				int Ox = static_cast<int>(tinyRay.O.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
				int Oy = static_cast<int>(tinyRay.O.z * m_debugSettings.scale.y + m_debugSettings.offset.y);

				int x = static_cast<int>((tinyRay.O.x + tinyRay.D.x * 5000) * m_debugSettings.scale.x + m_debugSettings.
					offset.x);
				int y = static_cast<int>((tinyRay.O.z + tinyRay.D.z * 5000) * m_debugSettings.scale.y + m_debugSettings.
					offset.y);

				//Draw the ray
				_debugScreen->Line(static_cast<float>(Ox), static_cast<float>(Oy), static_cast<float>(x),
				                   static_cast<float>(y), float4(1.f, 0.f, 0.f, 1.f));
				//Draw the normal
			}
		}
		if (_outNormal)
		{
			_outNormal->x = 0.5f;
			_outNormal->y = 0.5f;
			_outNormal->z = 0.5f;
			_outNormal->w = 1.0E30f;
		}
		if (_outPos)
		{
			_outPos->x = 0.f;
			_outPos->y = 0.f;
			_outPos->z = 0.f;
			_outPos->w = 1.f; //do not reproject
		}

		float4 skyColor = m_skyDome->m_texture->SampleSphere(tinyRay.D);
		out.albedo = skyColor;
		out.lighting = skyColor;
		out.length = 1.0e30f;

		CalculateContributionThroughVolumes(_outPos, _outNormal, out, tinyRay, MAX_BOUNCES);

		return out;
	}
	tinyRay.D = normalize(tinyRay.D);
	//Get hit info
	float2 hitUV(tinyRay.hit.u, tinyRay.hit.v);
	float3 I = tinyRay.O + tinyRay.D * tinyRay.hit.t;
	bool frontFace;
	float3 normal = m_renderScene->GetNormal(tinyRay.hit.inst, tinyRay.hit.prim, hitUV, _ray.D, I, frontFace,
	                                         m_mipMappingEnabled, m_lodBias,
	                                         m_camera.vFov, tinyRay.hit.t);

	float3 albedo = m_renderScene->GetAlbedo(tinyRay.hit.inst, tinyRay.hit.prim, hitUV, m_mipMappingEnabled, normal,
	                                         normalize(tinyRay.D),
	                                         tinyRay.hit.t, m_camera.vFov, m_lodBias);


	const Material& material = m_renderScene->GetMaterial(tinyRay.hit.inst, tinyRay.hit.prim, hitUV);

	float reflectivity = material.diellectric
		                     ? 0
		                     : m_renderScene->GetReflectivity(tinyRay.hit.inst, tinyRay.hit.prim, hitUV,
		                                                      m_mipMappingEnabled,
		                                                      normal,
		                                                      normalize(tinyRay.D),
		                                                      tinyRay.hit.t, m_camera.vFov, m_lodBias);
	float roughness = m_renderScene->GetRoughness(tinyRay.hit.inst, tinyRay.hit.prim, hitUV, m_mipMappingEnabled,
	                                              normal,
	                                              normalize(tinyRay.D),
	                                              tinyRay.hit.t, m_camera.vFov, m_lodBias);


	TraceOut metalOut;

	if (_bounces > 0 && reflectivity > 0.f && (!isDebug || _primaryDebugRay))
	{
		metalOut.lighting = out.lighting;
		metalOut.albedo = albedo;
		CalculateMettalic(_ray, _bounces, _outNormal, _outPos, _debugScreen, _primaryDebugRay, metalOut, tinyRay,
		                  metalOut.albedo, I,
		                  normal,
		                  roughness, reflectivity);
		if (reflectivity >= 1.f)
		{
			if (isDebug)
			{
				int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
				int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);

				if (_primaryDebugRay)
				{
					int Ox = static_cast<int>(tinyRay.O.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
					int Oy = static_cast<int>(tinyRay.O.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
					//Draw the ray

					float heightOffset = tinyRay.O.y - m_camera.camPos.y;
					float heightScale = 0.4f;
					float heightColor = heightOffset * heightScale;

					_debugScreen->Line(static_cast<float>(Ox), static_cast<float>(Oy), static_cast<float>(x),
					                   static_cast<float>(y),
					                   float4(max((heightColor) + 1.f, 0.3f), max(heightColor, 0.1f),
					                          max(heightColor, 0.1f), 1.f));
					//Draw the normal
					_debugScreen->Line(static_cast<float>(x), static_cast<float>(y),
					                   x + normal.x * m_debugSettings.normalScale,
					                   y + normal.z * m_debugSettings.normalScale,
					                   float4((normal + float3(1)) * 0.5f, 1));
				}

				Trace(Ray(I + tinyRay.D * EPSILON, tinyRay.D), 1, nullptr, nullptr, _debugScreen, false);

				x = clamp(x, 0, RENDER_WIDTH - 1);
				y = clamp(y, 0, RENDER_HEIGHT - 1);

				_debugScreen->m_pixels[x + y * RENDER_WIDTH] = float4(1.f);
			}
			out.albedo = metalOut.albedo;
			out.lighting = metalOut.lighting;

			CalculateContributionThroughVolumes(_outPos, _outNormal, out, tinyRay, MAX_BOUNCES);


			out.length = tinyRay.hit.t;
			return out;
		}
	}

	float transmission = material.transmission;
	//Based on raytracing in a weekend
	if (material.diellectric && transmission > 0.f && (!isDebug || _primaryDebugRay))
	{
		CalculateRefractive(_bounces, _outNormal, _outPos, _debugScreen, _primaryDebugRay, out, tinyRay, albedo, I,
		                    frontFace,
		                    normal, material, transmission, roughness);
	}
	else
	{
		//Choose either a diffuse reflection or direct sampling
		float f90 = min(1.f, 25.f * luminance(0.04f));
		float fresnel = evalFresnelSchlick(0.04f, f90, dot(normalize(-tinyRay.D), normal));
		float rand = SampleBlueNoise();
		if (_bounces > 0 && rand > roughness && rand < fresnel)
		{
			//Diffuse bounce
			float3 reflectDirection = reflect(_ray.D, normal);
			if (roughness > 0)
			{
				reflectDirection = normalize(reflectDirection + BlueRandomUnitVector());
				if (dot(reflectDirection, normal) < 0)
				{
					reflectDirection = normalize(reflect(_ray.D, normal));
				}
			}
			if (_debugScreen == nullptr || _primaryDebugRay)
			{
				float4 reflectNormal;
				float4 reflectPos;
				TraceOut reflectOut = Trace(Ray(I + reflectDirection * EPSILON, reflectDirection), _bounces - 1,
				                            &reflectNormal, &reflectPos, _debugScreen, _primaryDebugRay);

				if (_outNormal != nullptr)
				{
					_outNormal->x = normal.x;
					_outNormal->y = normal.y;
					_outNormal->z = normal.z;
					//normal A/w value contains depth info
					_outNormal->w = tinyRay.hit.t;
				}
				if (_outPos != nullptr)
				{
					_outPos->x = I.x;
					_outPos->y = I.y;
					_outPos->z = I.z;
					_outPos->w = 0.f; // Reproject
				}
				out.lighting = lerp(reflectOut.lighting * reflectOut.albedo, metalOut.lighting, reflectivity) +
					m_renderScene->GetEmission(tinyRay.hit.inst, tinyRay.hit.prim, hitUV);
			}
		}
		else
		{
			//Calculate lighting
			if (_debugScreen == nullptr || _primaryDebugRay)
				out.lighting = CalculateLighting(I, &normal, nullptr, _debugScreen);

			//return (normal+float3(1))*0.5f;
			if (_outNormal != nullptr)
			{
				_outNormal->x = normal.x;
				_outNormal->y = normal.y;
				_outNormal->z = normal.z;
				//normal A/w value contains depth info
				_outNormal->w = tinyRay.hit.t;
			}
			if (_outPos != nullptr)
			{
				_outPos->x = I.x;
				_outPos->y = I.y;
				_outPos->z = I.z;
				_outPos->w = 0.f; // Reproject
			}

			albedo = lerp(albedo, metalOut.albedo, reflectivity);
			out.lighting = lerp(out.lighting, metalOut.lighting, reflectivity) + m_renderScene->GetEmission(
				tinyRay.hit.inst, tinyRay.hit.prim, hitUV);
		}
	}
	if (isDebug)
	{
		int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);

		if (_primaryDebugRay)
		{
			int Ox = static_cast<int>(tinyRay.O.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
			int Oy = static_cast<int>(tinyRay.O.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
			//Draw the ray

			float heightOffset = tinyRay.O.y - m_camera.camPos.y;
			float heightScale = 0.4f;
			float heightColor = heightOffset * heightScale;

			_debugScreen->Line(static_cast<float>(Ox), static_cast<float>(Oy), static_cast<float>(x),
			                   static_cast<float>(y),
			                   float4(max((heightColor) + 1.f, 0.3f), max(heightColor, 0.1f),
			                          max(heightColor, 0.1f), 1.f));
			//Draw the normal
			_debugScreen->Line(static_cast<float>(x), static_cast<float>(y),
			                   x + normal.x * m_debugSettings.normalScale,
			                   y + normal.z * m_debugSettings.normalScale,
			                   float4((normal + float3(1)) * 0.5f, 1));
		}

		Trace(Ray(I + tinyRay.D * EPSILON, tinyRay.D), 1, nullptr, nullptr, _debugScreen, false);

		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);

		_debugScreen->m_pixels[x + y * RENDER_WIDTH] = float4(1.f);
	}

	out.albedo = albedo;

	CalculateContributionThroughVolumes(_outPos, _outNormal, out, tinyRay, MAX_BOUNCES);


	out.length = tinyRay.hit.t;
	return out;
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick(float _deltaTime)
{
	m_mouseDelta = m_lastMousePos - m_mousePos;
	m_lastMousePos = m_mousePos;

	AudioManager::GetAudioManager().Tick();

	// animation
	m_scene.SetTime(m_animTime += _deltaTime * 0.002f);

	if (!m_inMainMenu && !m_playerDied) m_renderScene->Tick(_deltaTime);
	else m_renderScene->Tick(0.f);
	GameTick(_deltaTime);

	m_renderScene->BuildTLAS();

	// pixel loop
	// lines are executed as OpenMP parallel tasks (disabled in DEBUG)
	if (m_debugSettings.debugDrawerEnabled)
	{
		debugScreen->Clear(float4(0, 0, 0, 1.f));
		int y = m_debugSettings.yLevel;
		// trace a primary ray for each pixel on the line
		for (int x = 0; x < RENDER_WIDTH; x++)
		{
			seed = InitSeed(m_debugSettings.seed);
			float4 normal;
			float4 pos;
			bool primary = x == m_debugSettings.primaryRay;
			Trace(m_camera.GetPrimaryRay(static_cast<float>(x), static_cast<float>(y), primary, seed), MAX_BOUNCES,
			      &normal, &pos, debugScreen, primary);
		}
	}

	if (!m_renderingPaused && !m_playerDied)
	{
#ifdef CALCULATE_ENERGY
		float totalEnergy = 0;
#endif


		m_accumulatorFrameCount++;
#pragma omp parallel for schedule(dynamic)  // NOLINT(clang-diagnostic-source-uses-openmp)
		for (int y = 0; y < RENDER_HEIGHT; y++)
		{
#ifdef LIMIT_THREADS
			SetThreadAffinityMask(GetCurrentThread(), THREAD_MASK);
#endif
#ifdef CALCULATE_ENERGY
			float energy = 0;
#endif
			// trace a primary ray for each pixel on the line
			for (int x = 0; x < RENDER_WIDTH; x++)
			{
				rayScreenPos = int2(x, y);
				blueNoiseSamples[rayScreenPos.x + rayScreenPos.y * RENDER_WIDTH] = static_cast<int>(screen->m_pixels[x +
					y *
					RENDER_WIDTH].w) % 8;
				if (seed == 0 || seed == m_debugSettings.seed) seed = InitSeed(x + y * RENDER_WIDTH);
				float4 normal;
				float4 pos;
				TraceOut result = Trace(
					m_camera.GetPrimaryRay(static_cast<float>(x), static_cast<float>(y), true, seed),
					MAX_BOUNCES, &normal, &pos);
				normals->m_pixels[x + y * RENDER_WIDTH] = normal;
				positions->m_pixels[x + y * RENDER_WIDTH] = pos;
				screen->m_pixels[x + y * RENDER_WIDTH] = float4(result.albedo, 1.0f);
				screenLighting->m_pixels[x + y * RENDER_WIDTH] = float4(result.lighting, 1.0f);
#ifdef CALCULATE_ENERGY
				energy += result.lighting.x + result.lighting.y + result.lighting.z;
#endif
			}
#ifdef CALCULATE_ENERGY
			totalEnergy += energy;
#endif
		}
#ifdef CALCULATE_ENERGY
		m_currentEnergy = totalEnergy;
#endif
	}
	if (m_debugSettings.debugDrawerEnabled)
	{
		//draw crosshair
		screen->Line(static_cast<float>(m_debugSettings.primaryRay + 1), 0.f,
		             static_cast<float>(m_debugSettings.primaryRay + 1), static_cast<float>(RENDER_HEIGHT),
		             float4(1.f, 0, 0, 1.f));
		screen->Line(static_cast<float>(m_debugSettings.primaryRay - 1), 0.f,
		             static_cast<float>(m_debugSettings.primaryRay - 1), static_cast<float>(RENDER_HEIGHT),
		             float4(1.f, 0, 0, 1.f));
		screen->Line(0.f, static_cast<float>(m_debugSettings.yLevel + 1), static_cast<float>(RENDER_WIDTH),
		             static_cast<float>(m_debugSettings.yLevel + 1), float4(1.f, 0, 0, 1.f));
		screen->Line(0.f, static_cast<float>(m_debugSettings.yLevel - 1), static_cast<float>(RENDER_WIDTH),
		             static_cast<float>(m_debugSettings.yLevel - 1), float4(1.f, 0, 0, 1.f));
	}


#ifdef HARD_FPS_LIMIT
	if (m_rawFrameTime > (1 / 30.f))
	{
		if (!m_fpsLimitExceeded) // First try a "subtle" approuch
		{
			m_aiming = false;
			m_camera.vFov = lerp(m_camera.vFov, VERTICAL_FOV, 0.5f);
		}
		else // Aggresively remove hard to render stuff
		{
			m_aiming = false;
			m_camera.vFov = VERTICAL_FOV;
			m_renderScene->m_volumeBVH->Clear();
		}
		m_fpsLimitExceeded = true;
	}
	else
	{
		m_fpsLimitExceeded = false;
	}
#endif

	// handle user input
	if (m_camera.HandleInput(_deltaTime))
	{
		m_accumulatorFrameCount = 0;
	}
	m_accumulatorFrameCount = 0;
}

//This should be inside another class not here
void Tmpl8::Renderer::GameTick(float _deltaTime)
{
	if (!m_startedMusic) AudioManager::GetAudioManager().PlayMusic("../Assets/Audio/mainMenu.mp3", 1.f);
	m_startedMusic = true;

	overlay->Clear(float4(0.f));

	GameObject** gameObjects = m_renderScene->GetGameObjects();
	for (uint i = 0; i <= m_renderScene->GetObjectAmount(); ++i)
	{
		GameObject* object = gameObjects[i];
		if (object == nullptr) continue;
		if (!m_debugCamera && object->GetName().rfind("camera", 0) == 0)
		{
			if (m_cameraObject == nullptr) m_cameraObject = object;
			m_camera.camPos = object->GetPosition();
			quat rot = object->GetRotation();
			if (!m_inMainMenu)
			{
				quat offsetRotY;
				offsetRotY.fromAxisAngle(float3(0, 1, 0), m_cameraRotationOffset.y);
				quat offsetRotx;
				offsetRotx.fromAxisAngle(float3(1, 0, 0), m_cameraRotationOffset.x);
				rot = rot * offsetRotY * offsetRotx;
			}
			m_camera.camTarget = m_camera.camPos + float3(rot.toMatrix() * float3(0, 0, -1));
			m_camera.CalcView();
		}
		else if (object->GetName().rfind("gun", 0) == 0)
		{
			if (m_gunObject == nullptr) m_gunObject = object;

			if (m_cameraObject != nullptr && m_inCamera)
			{
				quat gunRot = m_cameraObject->GetRotation();
				quat offsetRotY;
				offsetRotY.fromAxisAngle(float3(0, 1, 0), m_cameraRotationOffset.y);
				quat offsetRotx;
				offsetRotx.fromAxisAngle(float3(1, 0, 0), m_cameraRotationOffset.x);
				quat extraOffset;
				extraOffset.fromAxisAngle(float3(1, 0, 0), PI * 0.5f);
				gunRot = gunRot * offsetRotY * offsetRotx * extraOffset;
				object->SetRotation(gunRot);
				object->SetPosition(
					m_cameraObject->GetPosition() + gunRot.rotateVector(
						float3(0, -m_gunOffset, 0)));
				object->UpdateTransform();

				m_gunOffset = lerp(m_gunOffset, 0.f, min(_deltaTime * 0.01f, 1.f));
			}
		}
	}

	//UI
	if (m_playerDied)
	{
		m_cursorLockMode = GLFW_CURSOR_NORMAL;
		overlay->Clear(float4(0.2f, 0.f, 0.f, 1.f));
		overlay->Print("MISSION FAILED", UI_WIDTH / 2 - 35, 20, float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
		overlay->Print(("SCORE: " + std::to_string(ScoreManager::GetScoreManager()->GetScore())).c_str(),
		               UI_WIDTH / 2 - 35, 30,
		               float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
		overlay->Print(
			("KILLS: " + std::to_string(ScoreManager::GetScoreManager()->GetEnemiesKilled()) + "/" + std::to_string(
				ScoreManager::GetScoreManager()->GetEnemiesTotal())).c_str(),
			UI_WIDTH / 2 - 35, 40,
			float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
		if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
			&& m_uiMousePos.y > UI_HEIGHT - 120 && m_uiMousePos.y < UI_HEIGHT - 100)
		{
			overlay->Print("  [ Restart ]", UI_WIDTH / 2 - (6 * 7), 110, float4(1.f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
		else
		{
			overlay->Print("  [Restart]", UI_WIDTH / 2 - (6 * 6), 110, float4(0.8f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
		if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
			&& m_uiMousePos.y > UI_HEIGHT - 140 && m_uiMousePos.y < UI_HEIGHT - 120)
		{
			overlay->Print("  [ Quit ]", UI_WIDTH / 2 - (6 * 6), 130, float4(1.f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
		else
		{
			overlay->Print("  [Quit]", UI_WIDTH / 2 - (6 * 5), 130, float4(0.8f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
	}
	else if (m_gameFinished)
	{
		overlay->Clear(float4(0.f, 0.2f, 0.f, 0.1f));
		if (m_cameraObject->GetAnimTime() > 3270.f * 41.66667f)
		{
			if (!m_spawnedEndsplosion)
			{
				m_spawnedEndsplosion = true;
				Volume volume;
				volume.pos = float3(0, -105, 0);
				volume.r = 7.f;
				volume.rEnd = 10.f;

				volume.albedo = float3(0.1f);
				volume.emission = float3(1000.f, 200.f, 20.f);
				volume.emissionDensitySubtraction = 0.1f;
				volume.absorption = 1.9f;
				volume.scattering = 1.9f;
				volume.noiseScale = 18.237f;
				volume.faloffStart = 0.6f;
				volume.innerDensityAddition = 0.2f;
				volume.animationSpeed = 238.2f;
				m_renderScene->m_volumeBVH->SpawnFromPool(5000.f, volume);
			}
			//overlay->Print("MISSION WIN", UI_WIDTH / 2 - 29, 20, float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
			overlay->Print(("SCORE: " + std::to_string(ScoreManager::GetScoreManager()->GetScore())).c_str(),
			               UI_WIDTH / 2 - 29, 80,
			               float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
			overlay->Print(
				("KILLS: " + std::to_string(ScoreManager::GetScoreManager()->GetEnemiesKilled()) + "/" + std::to_string(
					ScoreManager::GetScoreManager()->GetEnemiesTotal())).c_str(),
				UI_WIDTH / 2 - 29, 90,
				float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
			if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
				&& m_uiMousePos.y > UI_HEIGHT - 120 && m_uiMousePos.y < UI_HEIGHT - 100)
			{
				overlay->Print("  [ Restart ]", UI_WIDTH / 2 - (6 * 7), 110, float4(1.f, 0.f, 0.f, 1.f),
				               float3(0.f));
			}
			else
			{
				overlay->Print("  [Restart]", UI_WIDTH / 2 - (6 * 6), 110, float4(0.8f, 0.f, 0.f, 1.f),
				               float3(0.f));
			}
			if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
				&& m_uiMousePos.y > UI_HEIGHT - 140 && m_uiMousePos.y < UI_HEIGHT - 120)
			{
				overlay->Print("  [ Quit ]", UI_WIDTH / 2 - (6 * 6), 130, float4(1.f, 0.f, 0.f, 1.f),
				               float3(0.f));
			}
			else
			{
				overlay->Print("  [Quit]", UI_WIDTH / 2 - (6 * 5), 130, float4(0.8f, 0.f, 0.f, 1.f),
				               float3(0.f));
			}
		}
	}
	else if (m_inMainMenu)
	{
		overlay->Bar(UI_WIDTH / 2 - 30, 17, UI_WIDTH / 2 + 30, 27, float4(0.f, 0.f, 0.f, 0.7f));
		overlay->Print("HeliTrace", UI_WIDTH / 2 - 25, 20, float4(1.f, 0.f, 0.f, 1.f),
		               float4(0.f, 0.f, 0.f, 1.f));
		if (m_uiMousePos.x > UI_WIDTH / 2 - (5 * 5) && m_uiMousePos.x < UI_WIDTH / 2 + (5 * 5)
			&& m_uiMousePos.y > 35 && m_uiMousePos.y < 50)
		{
			overlay->Print("[ START ]", UI_WIDTH / 2 - (5 * 6) - 1, UI_HEIGHT - 45, float4(1.f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
		else
		{
			overlay->Print("[START]", UI_WIDTH / 2 - (5 * 5), UI_HEIGHT - 45, float4(0.8f, 0.f, 0.f, 1.f),
			               float3(0.f));
		}
		overlay->Bar(UI_WIDTH / 2 - (3 * 25), 29, UI_WIDTH / 2 + (3 * 22), 36, float4(0.f, 0.f, 0.f, 0.7f));
		overlay->Print("destroy the 6G tower!!!", UI_WIDTH / 2 - (3 * 24), 30, float4(0.5f, 0.f, 0.f, 1.f),
		               float4(0.f, 1.f));
		overlay->Print("Q to quit", UI_WIDTH - 53, 1, float4(1.f, 0.f, 0.f, 1.f), float3(0.f));

		std::string difficultyName;
		switch (m_difficulty)
		{
		case Difficulty::GameReviewer:
			difficultyName = "Game Reviewer";
			break;
		case Difficulty::Normal:
			difficultyName = "Normal";
			break;
		case Difficulty::Hard:
			difficultyName = "Hard";
			break;
		case Difficulty::EldenRing:
			difficultyName = "Elden Ring";
			break;
		}
		if (m_uiMousePos.x > 0 && m_uiMousePos.x < 12 * 6
			&& m_uiMousePos.y > UI_HEIGHT - 15 && m_uiMousePos.y < UI_HEIGHT)
		{
			overlay->Print(("Difficulty:" + difficultyName).c_str(), 0, 1, float4(1.f, 0.f, 0.f, 1.f), float3(0.f));
		}
		else
		{
			overlay->Print(("Difficulty:" + difficultyName).c_str(), 0, 1, float4(0.8f, 0.f, 0.f, 1.f), float3(0.f));
		}

		//Logo
		static const bool logo[28][28]
		{
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
		};
		int sX = 1;
		int sY = UI_HEIGHT - 30;
		for (int y = 0; y < 28; ++y)
		{
			for (int x = 0; x < 28; ++x)
			{
				overlay->Pixel(sX + x, sY + y, !logo[y][x] ? float4(0.93f, 0.1f, 0.08f, 1.f) : float4(0.f));
			}
		}
		overlay->Print("Made by Jack Tollenaar", 68, UI_HEIGHT - 5, float4(1.f, 0.f, 0.4f, 0.4f), float4(0.f));

		if (m_uiMousePos.x > UI_WIDTH - 9 && m_uiMousePos.x < UI_WIDTH - 1
			&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
		{
			overlay->Print("MUS", UI_WIDTH - 18, UI_HEIGHT - 58, float4(1.f, 0, 0, 1.f), float4(0.f));
		}
		else if (m_uiMousePos.x > UI_WIDTH - 17 && m_uiMousePos.x < UI_WIDTH - 9
			&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
		{
			overlay->Print("SFX", UI_WIDTH - 18, UI_HEIGHT - 58, float4(1.f, 0, 0, 1.f), float4(0.f));
		}
		float sfxVolume = AudioManager::GetAudioManager().GetSfxVol();
		overlay->Bar(UI_WIDTH - 16, UI_HEIGHT - 50, UI_WIDTH - 10, UI_HEIGHT - 8, float4(0.f, 0.f, 0.f, 0.7f));
		overlay->Bar(UI_WIDTH - 16, static_cast<int>(round(UI_HEIGHT - 50 * sfxVolume)),UI_WIDTH - 10, UI_HEIGHT - 8,
		             float4(1.f, 0.f, 0.f, 1.f));
		float musVolume = AudioManager::GetAudioManager().GetMusicVol();
		overlay->Bar(UI_WIDTH - 8, UI_HEIGHT - 50, UI_WIDTH - 2, UI_HEIGHT - 8, float4(0.f, 0.f, 0.f, 0.7f));
		overlay->Bar(UI_WIDTH - 8, static_cast<int>(round(UI_HEIGHT - 50 * musVolume)), UI_WIDTH - 2, UI_HEIGHT - 8,
		             float4(1.f, 0.f, 0.f, 1.f));
	}
	else
	{
		if (m_inCamera)
		{
			// UI
			int2 center = int2(static_cast<int>(UI_WIDTH * 0.5f), static_cast<int>(UI_HEIGHT * 0.5f));
			overlay->Line(static_cast<float>(center.x - 3), static_cast<float>(center.y),
			              static_cast<float>(center.x + 3),
			              static_cast<float>(center.y), float4(1, 1, 1, -1));
			overlay->Line(static_cast<float>(center.x), static_cast<float>(center.y - 3), static_cast<float>(center.x),
			              static_cast<float>(center.y + 3), float4(1, 1, 1, -1));

			overlay->Print(std::to_string(ScoreManager::GetScoreManager()->GetScore()).c_str(), 0, 0,
			               float4(1, 0, 0, 0.5f), float3(0.f));

			int2 barSize(UI_WIDTH - 5, 3);
			float health = m_heliHealth / m_heliMaxHealth;
			float4 barColor = float4(0.5f, 0, 0, 1.f);
			if (health < 0.15f)
			{
				if (m_playedDamageAlert <= 1)
				{
					AudioManager::GetAudioManager().PlaySoundFile(
						"../assets/Audio/closeDeath.wav", false, 0.1f,
						false);
					m_playedDamageAlert = 2;
				}
				barColor = float4(sin(m_animTime * 6.f) * 0.8f + 0.2f, 0.f, 0.f, 1.f);
				overlay->Print("DESTRUCTION IMMINENT:", 3, UI_HEIGHT - 14, barColor,
				               barColor * 0.5f);
			}
			else if (health < 0.3f)
			{
				if (m_playedDamageAlert == 0)
				{
					AudioManager::GetAudioManager().PlaySoundFile(
						"../assets/Audio/damageAlert.wav", false, 1.f,
						true, m_cameraObject->GetPosition() +
						m_cameraObject->GetRotation().rotateVector(
							float3(0, 2, -3)), 0.f);
					m_playedDamageAlert = 1;
				}
				barColor = float4(sin(m_animTime * 3.f) * 0.5f + 0.5f, 0.f, 0.f, 1.f);
				overlay->Print("CRITICAL Integrity:", 3, UI_HEIGHT - 14, barColor,
				               barColor * 0.5f);
			}
			else if (health < 0.5f)
			{
				barColor = float4(sin(m_animTime * 1.5f) * 0.2f + 0.8f, 0.f, 0.f, 1.f);
				overlay->Print("LOW Integrity:", 3, UI_HEIGHT - 14, barColor,
				               barColor * 0.5f);
			}
			else
			{
				overlay->Print("Integrity:", 3, UI_HEIGHT - 14, float4(1.f, 0, 0, 0.7f), float4(0.f, 0.f, 0.f, 0.f));
			}
			overlay->Bar(2, UI_HEIGHT - 8, static_cast<int>(2 + health * barSize.x), UI_HEIGHT - 8 + barSize.y,
			             barColor);
		}

		//Game logic
		if (m_inCamera)
		{
			if (m_aiming)
			{
				if (static_cast<int>(m_camera.vFov) != static_cast<int>(AIM_FOV))
				{
					m_camera.vFov = lerp(m_camera.vFov, AIM_FOV, min(_deltaTime * 0.01f, 1.f));
					m_camera.CalcView();
				}
			}
			else
			{
				if (static_cast<int>(m_camera.vFov) != static_cast<int>(VERTICAL_FOV))
				{
					m_camera.vFov = lerp(m_camera.vFov, VERTICAL_FOV, min(_deltaTime * 0.01f, 1.f));
					m_camera.CalcView();
				}
			}
		}

		if (m_triggerShoot)
		{
			m_triggerShoot = false;
			PlayerShoot();
		}

		m_cameraRotationOffset.y += m_mouseDelta.x * MOUSE_SENSITIVITY;
		m_cameraRotationOffset.x -= m_mouseDelta.y * MOUSE_SENSITIVITY;
		m_cameraRotationOffset.y = clamp(m_cameraRotationOffset.y, -PI * 0.4f + EPSILON, PI * 0.4f - EPSILON);
		m_cameraRotationOffset.x = clamp(m_cameraRotationOffset.x, -PI * 0.4f + EPSILON, PI * 0.4f - EPSILON);


		if (m_inCamera && !AudioManager::GetAudioManager().IsPlaying(m_heliStartupSound))
		{
			if (m_heliSound == 0)
			{
				m_heliSound = AudioManager::GetAudioManager().PlaySoundFile(
					"../assets/Audio/helicopter.wav", true, 4.5f,
					true, m_cameraObject->GetPosition() +
					m_cameraObject->GetRotation().rotateVector(
						float3(0, 2, -3)));
			}
			else
			{
				float3 soundPos = m_cameraObject->GetPosition() + m_cameraObject->GetRotation().rotateVector(
					float3(0, 2, -3));
				ma_sound_set_position(AudioManager::GetAudioManager().GetSound(m_heliSound), soundPos.x, soundPos.y,
				                      soundPos.z);
			}
		}

		if (m_heliStartupSound == 0)
		{
			m_heliStartupSound = AudioManager::GetAudioManager().PlaySoundFile(
				"../assets/Audio/helistartup.wav", false, 4.5f, true, float3(22, 6, -45));
		}

		//Switch to camera anim
		if (!m_inCamera && m_cameraObject->GetAnimTime() > 250.f * 41.66667f)
		{
			AudioManager::GetAudioManager().PlayMusic("../assets/Audio/gameplay.mp3");
			m_inCamera = true;
			abberation = true;
			distortion = 0.145f;
			vignetting = true;
			vignetteSize = 7.2f;
			vignetteSmooth = 0.29f;
			CRTDistort = true;
			scanLines = false;
		}
		//End state
		if (m_cameraObject->GetAnimTime() > 2880.f * 41.66667f)
		{
			AudioManager::GetAudioManager().StopAll();
			AudioManager::GetAudioManager().PlaySoundFile(
				"../assets/Audio/winexplode.wav", false, 0.1f, false);
			AudioManager::GetAudioManager().PlayMusic("../assets/Audio/victory.mp3");
			m_cursorLockMode = GLFW_CURSOR_NORMAL;

			abberation = false;
			distortion = 0.145f;
			vignetting = true;
			vignetteSize = 4.2f;
			vignetteSmooth = 0.29f;
			CRTDistort = false;
			scanLines = false;
			m_inCamera = false;
			m_cameraRotationOffset = float2(0);

			m_gameFinished = true;
		}
	}

#ifdef DISPLAY_FPS
	int fpsPosY = m_inMainMenu ? 8 : 1;
	overlay->Print(std::to_string(static_cast<int>(m_fps)).c_str(), UI_WIDTH - 18, fpsPosY, float4(1.f, 0.f, 0.f, 0.5f),
	               float4(0.f));
#endif
}

// -----------------------------------------------------------
// Update user interface (imgui)
// -----------------------------------------------------------
void Renderer::UI()
{
#ifdef DEBUG_UI
	ImGui::BeginMainMenuBar();


	float4 high, low;
	float highTres = 30;
	float lowTres = 0;
	high = float4(1, 1, 1, 1.f);
	low = float4(1, 0.2f, 0.f, 1.f);
	float4 color = lerp(low, high, (m_fps - lowTres) / (highTres - lowTres));

	auto c = ImVec4(color.x, color.y, color.z, color.w);
	ImGui::TextColored(c, "%5.2fms (%.1ffps)\n", m_avg, m_fps);

	ImGui::SetCursorPosX(250.f);
	bool rendering = !m_renderingPaused;
	if (ImGui::Checkbox("Render", &rendering)) m_renderingPaused = !rendering;

#ifdef CALCULATE_ENERGY
	ImGui::SetCursorPosX(ImGui::GetMainViewport()->Size.x - 200.f);
	m_oldEnergy = lerp(m_oldEnergy, m_currentEnergy, 0.8f);
	ImGui::Text("Energy: %f", m_oldEnergy * 0.001f);
#endif

	ImGui::EndMainMenuBar();

	ImGui::DockSpaceOverViewport(1u);


	ImGui::Begin("Debug View");
	auto imagSize = ImVec2((ImGui::GetWindowSize().x * 0.7f), (ImGui::GetWindowSize().x * 0.7f) * (1 / ASPECT_RATIO));
	ImGui::Image(debugRenderTexture, imagSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::SameLine();
	ImGui::BeginChild("Debug settings", ImVec2(0, 0), ImGuiChildFlags_Borders);
	if (ImGui::Checkbox("Debug view", &m_debugSettings.debugDrawerEnabled))
	{
	}
	if (m_debugSettings.debugDrawerEnabled)
	{
		if (ImGui::TreeNode("Legend"))
		{
			ImGui::Text("White dots, environment");
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Red");
			ImGui::SameLine();
			ImGui::Text("Primary/Secondary rays");
			ImGui::Text("Y level is shown by lightness of the ray (lighter higher, darker lower)");
			ImGui::Text("Normals are shown at intersections with colors based on direction");
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 1, 1), "(light)");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 0, 1, 1), "Blue");
			ImGui::SameLine();
			ImGui::Text(" Shadow rays");


			ImGui::TreePop();
		}
		if (ImGui::Button("Break")) m_debugSettings.breakNext = true;
		ImGui::SliderInt("SelectedRay", &m_debugSettings.primaryRay, 0, RENDER_WIDTH - 1);
		ImGui::SliderInt("Selected Y", &m_debugSettings.yLevel, 0, RENDER_HEIGHT - 1);
		int setSeed = static_cast<int>(m_debugSettings.seed);
		if (ImGui::SliderInt("Random seed", &setSeed, 1, 200))
		{
			m_debugSettings.seed = static_cast<uint>(setSeed);
		}
		ImGui::SliderInt2("Offset", &m_debugSettings.offset.x, -1000, 1000);
		float scale = m_debugSettings.scale.x;
		if (ImGui::SliderFloat("Scale", &scale, -100, 100))
		{
			m_debugSettings.scale = float2(scale);
		}
		ImGui::SliderFloat("NormalScale", &m_debugSettings.normalScale, -100, 100);
	}
	ImGui::EndChild();

	ImGui::End();

	ImGui::Begin("settings");
	ImGui::Checkbox("Debug Camera", &m_debugCamera);
	if (ImGui::TreeNode("Image settings"))
	{
		ImGui::Checkbox("Blue noise", &m_sampleBlueNoise);
		ImGui::Checkbox("MipMapping", &m_mipMappingEnabled);
		if (m_mipMappingEnabled)
		{
			ImGui::SliderFloat("Lod Bias", &m_lodBias, -10.f, 10.f);
		}
		if (ImGui::TreeNode("Reprojection and accumulation"))
		{
			ImGui::SliderFloat("Accumulator lerp amount", &accumulatorLerp, 0.f, 1.f);
			ImGui::SliderFloat("Pixel change treshold", &pixelChangeTreshold, 0.f, 1.f);
			ImGui::Checkbox("Reproject", &reproject);
			if (reproject)
			{
				ImGui::SliderFloat("Accumulator reprojection difference influence", &accumulatorLerpDiff, 0.f, 200.f);
				ImGui::SliderFloat("Reproject similarity treshold", &reprojectSimilarityTreshold, 0.f, 10.f);
			}
			ImGui::Checkbox("Force accumulate", &forceAccumulate);
			ImGui::Checkbox("Force update", &forceUpdate);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Depth of field"))
		{
			bool depthOfField = m_camera.defocusAngle > 0.00f;
			if (ImGui::Checkbox("Traced Depth of Field", &depthOfField))
				if (!depthOfField) m_camera.defocusAngle = 0.f;
				else m_camera.defocusAngle = 1.f;
			if (depthOfField)
			{
				if (ImGui::SliderFloat("Defocus angle", &m_camera.defocusAngle, 0.001f, 20.f))
				{
					m_accumulatorFrameCount = 0;
					m_camera.SetFocusDistance(m_camera.GetFocusDistance()); // recalc focusRange
				}
				float defocusDist = m_camera.GetFocusDistance();
				if (ImGui::SliderFloat("Focus distance", &defocusDist, 0.01f, 20.f, "%.2f"))
				{
					m_accumulatorFrameCount = 0;
					m_camera.SetFocusDistance(defocusDist);
				}
			}
			bool postDepthOfField = postDofAmount > 0.00f;
			if (ImGui::Checkbox("PostProcess Depth of Field", &postDepthOfField))
				if (!postDepthOfField) postDofAmount = 0.f;
				else postDofAmount = 1.f;
			if (postDepthOfField)
			{
				ImGui::SliderFloat("Blur Amount", &postDofAmount, 0.001f, 5.f);
				ImGui::SliderFloat("Blur falloff", &postDofFalloff, 0.001f, 1.f);
				ImGui::SliderFloat("Focus distance", &postDofFocusDist, 0.01f, 20.f, "%.2f");
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Post processing"))
		{
			ImGui::Checkbox("Chromatic Abberation", &abberation);
			if (abberation) ImGui::SliderFloat("  Distortion", &distortion, 0.f, 25.f);
			ImGui::Checkbox("Tonemapping", &tonemap);
			if (tonemap) ImGui::SliderFloat("  Exposure", &exposure, 0.f, 2.f);
			ImGui::Checkbox("Vignette", &vignetting);
			if (vignetting) ImGui::SliderFloat("  Size", &vignetteSize, 0.f, 50.f);
			if (vignetting) ImGui::SliderFloat("  Smoothing", &vignetteSmooth, 0.f, 1.f);
		}
		ImGui::Checkbox("Denoise", &denoise);
		ImGui::SliderFloat("pos falloff", &posFallSpeed, 0.f, 10.f);
		ImGui::SliderFloat("edge falloff", &edgeFallSpeed, 0.f, 10.f);

		if (ImGui::SliderFloat("Vertical FOV", &m_camera.vFov, 10.f, 180.f))
		{
			m_camera.CalcView();
			m_accumulatorFrameCount = 0;
		}

		if (ImGui::Checkbox("Panini projection", &m_camera.paniniProjected)) m_accumulatorFrameCount = 0;
		if (m_camera.paniniProjected)
		{
			if (ImGui::SliderFloat("Strength", &m_camera.A, 1.f, 3.f)) m_accumulatorFrameCount = 0;
		}


		ImGui::TreePop();

		//Dock things to main
	}
	// animation toggle
	// ray query on mouse


	ImGui::Begin("Materials");
	if (ImGui::TreeNode("Scene Materials"))
	{
		MaterialManager& materialManager = m_renderScene->GetMaterialManager();
		const uint materialCount = materialManager.GetMaterialCount();
		for (uint i = 0; i < materialCount; i++)
		{
			if (ImGui::TreeNode(std::to_string(i).c_str()))
			{
				Material mat = materialManager.GetMaterial(i);
				bool changed = false;
				if (mat.hasTextures)
				{
					ImGui::Text("Albedo Texture: %d", mat.albedoTexture);
					ImGui::Text("Normal Texture: %d", mat.normalTexture);
					ImGui::Text("pbr Texture: %d", mat.metallicRoughnessTexture);
				}
				else
				{
					if (ImGui::ColorPicker3("Albedo", &mat.albedo.x)) changed = true;
				}
				if (ImGui::Checkbox("Is Dielectric", &mat.diellectric)) changed = true;
				if (mat.diellectric)
				{
					if (ImGui::SliderFloat("Transmission", &mat.transmission, 0.f, 1.f)) changed = true;
					if (ImGui::SliderFloat("Density", &mat.density, 0.f, 30.f)) changed = true;
					if (ImGui::InputFloat("IOR", &mat.IOR, 0.05f, 0.1f)) changed = true;
				}
				else
				{
					if (ImGui::SliderFloat("Mettalicness", &mat.metallic, 0.f, 1.f)) changed = true;
				}
				if (ImGui::SliderFloat("Roughness", &mat.roughness, 0.f, 10.f)) changed = true;


				if (changed) materialManager.SetMaterial(mat, i);

				ImGui::TreePop();
			}
		}


		ImGui::TreePop();
	}


	auto scale = float2(1.f);
	// float2(renderImageSize.x, renderImageSize.y) / float2(WINDOW_WIDTH, WINDOW_HEIGTH);
	auto RenderMousePos = float2((m_mousePos.x) * scale.x, (m_mousePos.y) * scale.y);

	Ray r = m_camera.GetPrimaryRay(RenderMousePos.x, RenderMousePos.y, false, seed);
	auto ray = tinybvh::Ray(tinybvh::bvhvec3(r.O.x, r.O.y, r.O.z), tinybvh::bvhvec3(r.D.x, r.D.y, r.D.z));
	m_renderScene->FindNearest(ray);
	ImGui::Text("Object id: %i", ray.hit.prim);
	if (ray.hit.prim != 0)
	{
		//const Vertex& hitVertice = renderScene.GetVertexData(ray.hit.inst, ray.hit.prim);
		//ImGui::Text("texture: %i", hitVertice.texture);
		//ImGui::Text("  texCoords: %f, %f", hitVertice.TexCoords.x, hitVertice.TexCoords.y);

		if (IsKeyDown(GLFW_KEY_1))
		{
			//Testing feature set DOF to mouse pos
			float newFocus = ray.hit.t;
			float oldFocus = m_camera.GetFocusDistance();
			if (abs(oldFocus - newFocus) > 0.5f)
			{
				m_accumulatorFrameCount = 0;
				m_camera.SetFocusDistance(lerp(oldFocus, newFocus, 0.8f)); // framerate dependent
			}
		}
	}
	if (IsKeyDown(GLFW_KEY_E))
	{
		//enable debug for this pixel
		m_debugSettings.debugDrawerEnabled = true;
		m_debugSettings.primaryRay = m_mousePos.x;
		m_debugSettings.yLevel = m_mousePos.y;
	}

	ImGui::End();
#else

#endif
}

void Renderer::Shutdown()
{
	delete m_skyDome;
	delete m_renderScene;
}

void Tmpl8::Renderer::MouseUp(int _button)
{
	if (_button == 1)
	{
		m_aiming = false;
		if (m_inMainMenu)
		{
			if (m_uiMousePos.x > UI_WIDTH - 9 && m_uiMousePos.x < UI_WIDTH - 1
				&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
			{
				auto& audioMan = AudioManager::GetAudioManager();
				float vol = audioMan.GetMusicVol() - 0.1f;
				if (vol < 0) vol = 1.f;
				audioMan.SetMusicVol(vol);
			}
			else if (m_uiMousePos.x > UI_WIDTH - 17 && m_uiMousePos.x < UI_WIDTH - 9
				&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
			{
				auto& audioMan = AudioManager::GetAudioManager();
				float vol = audioMan.GetSfxVol() - 0.1f;
				if (vol < 0) vol = 1.f;
				audioMan.SetSfxVol(vol);
			}
		}
	}
	if (_button == 0)
	{
		if (m_inMainMenu)
		{
			if (m_uiMousePos.x > UI_WIDTH / 2 - (5 * 5) && m_uiMousePos.x < UI_WIDTH / 2 + (5 * 5)
				&& m_uiMousePos.y > 35 && m_uiMousePos.y < 50)
			{
				//Clicked start

				m_inMainMenu = false;
				m_cursorLockMode = GLFW_CURSOR_DISABLED;
			}
			if (m_uiMousePos.x > 0 && m_uiMousePos.x < 12 * 6
				&& m_uiMousePos.y > UI_HEIGHT - 7 && m_uiMousePos.y < UI_HEIGHT)
			{
				switch (m_difficulty)
				{
				case Difficulty::EldenRing:
					SetDifficulty(Difficulty::GameReviewer);
					break;
				case Difficulty::GameReviewer:
					SetDifficulty(Difficulty::Normal);
					break;
				case Difficulty::Normal:
					SetDifficulty(Difficulty::Hard);
					break;
				case Difficulty::Hard:
					SetDifficulty(Difficulty::EldenRing);
					break;
				}
			}
			if (m_uiMousePos.x > UI_WIDTH - 9 && m_uiMousePos.x < UI_WIDTH - 1
				&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
			{
				auto& audioMan = AudioManager::GetAudioManager();
				float vol = audioMan.GetMusicVol() + 0.1f;
				if (vol > 1.09f) vol = 0.f;
				audioMan.SetMusicVol(vol);
			}
			else if (m_uiMousePos.x > UI_WIDTH - 17 && m_uiMousePos.x < UI_WIDTH - 9
				&& m_uiMousePos.y > 4 && m_uiMousePos.y < 54)
			{
				auto& audioMan = AudioManager::GetAudioManager();
				float vol = audioMan.GetSfxVol() + 0.1f;
				if (vol > 1.09f) vol = 0.f;
				audioMan.SetSfxVol(vol);
			}
		}
		else if (m_gameFinished || m_playerDied)
		{
			if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
				&& m_uiMousePos.y > UI_HEIGHT - 120 && m_uiMousePos.y < UI_HEIGHT - 100)
			{
				m_running = false;
			}
			if (m_uiMousePos.x > UI_WIDTH / 2 - (6 * 7) && m_uiMousePos.x < UI_WIDTH / 2 + (6 * 9)
				&& m_uiMousePos.y > UI_HEIGHT - 140 && m_uiMousePos.y < UI_HEIGHT - 120)
			{
				m_quit = true;
			}
		}
	}
}

void Tmpl8::Renderer::MouseDown(int _button)
{
	if (_button == 0 && m_inCamera)
	{
		m_triggerShoot = true;
	}
	else if (_button == 1)
	{
		m_aiming = true;
	}
}

void Tmpl8::Renderer::KeyUp(int)
{
}

void Tmpl8::Renderer::KeyDown(int _key)
{
	if (_key == GLFW_KEY_Q)
	{
		m_quit = true;
	}
	if (_key == GLFW_KEY_R)
	{
		m_running = false;
	}

	if (m_inMainMenu && _key == GLFW_KEY_F)
	{
		switch (m_difficulty)
		{
		case Difficulty::EldenRing:
			SetDifficulty(Difficulty::GameReviewer);
			break;
		case Difficulty::GameReviewer:
			SetDifficulty(Difficulty::Normal);
			break;
		case Difficulty::Normal:
			SetDifficulty(Difficulty::Hard);
			break;
		case Difficulty::Hard:
			SetDifficulty(Difficulty::EldenRing);
			break;
		}
	}
	if (_key == GLFW_KEY_ESCAPE)
	{
		m_inMainMenu = true;
		m_cursorLockMode = GLFW_CURSOR_NORMAL;
	}

	if (_key == GLFW_KEY_SPACE)
	{
		m_inMainMenu = false;
		m_cursorLockMode = GLFW_CURSOR_DISABLED;
	}
	if (_key == GLFW_KEY_O)
	{
		m_renderScene->Tick(10000.f);
	}
	if (_key == GLFW_KEY_P)
	{
		m_renderScene->Tick(100000.f);
	}

	if (_key == GLFW_KEY_G)
	{
		m_debugCamera = true;
		if (m_renderScene->m_volumeBVH != nullptr)
		{
			for (int i = 0; i < 1; ++i)
			{
				m_renderScene->m_volumeBVH->m_volumes[i].lifeTime = float2(
					m_renderScene->m_volumeBVH->GetCurrentAnimTime(),
					m_renderScene->m_volumeBVH->GetCurrentAnimTime() + 90000.f);
				m_renderScene->m_volumeBVH->m_volumes[i].pos = float3(RandomFloat() * 10.f, 3.f, RandomFloat() * 10.f);
			}
		}
	}
	if (_key == GLFW_KEY_TAB) // mouse lock
	{
		if (m_cursorLockMode == GLFW_CURSOR_DISABLED)
		{
			m_cursorLockMode = GLFW_CURSOR_NORMAL;
		}
		else
		{
			m_cursorLockMode = GLFW_CURSOR_DISABLED;
		}
	}
}

void Tmpl8::Renderer::DisplayImage(std::string _type, uint _texture, bool _flip, bool /*_main*/)
{
	ImGui::SetNextWindowDockID(1u, ImGuiCond_Appearing);
	ImGui::Begin(_type.c_str());
	ImVec2 renderImageSize;
	ImVec2 renderImagePos;
	if ((ImGui::GetWindowHeight() - 60) * ASPECT_RATIO < ImGui::GetWindowWidth())
	{
		renderImageSize = ImVec2((ImGui::GetWindowSize().y - 60) * ASPECT_RATIO, (ImGui::GetWindowSize().y - 60));
		renderImagePos.x = 0;
		renderImagePos.y = 50;
	}
	else
	{
		renderImageSize = ImVec2((ImGui::GetWindowSize().x), (ImGui::GetWindowSize().x) * 1 / ASPECT_RATIO);
		renderImagePos.x = 0;
		renderImagePos.y = 50;
	}

	ImGui::SetCursorPosX(0);
	ImGui::SetCursorPosY(50);
	if (_flip)
		ImGui::Image(_texture, renderImageSize, ImVec2(0, 1), ImVec2(1, 0));
	else
		ImGui::Image(_texture, renderImageSize, ImVec2(0, 0), ImVec2(1, 1));

	ImGui::End();
}

void Tmpl8::Renderer::PlayerShoot()
{
	m_gunOffset = 1.0f;

	AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/gun.wav", false, 0.1f, true,
	                                              m_gunObject->GetPosition() + m_gunObject->GetRotation().rotateVector(
		                                              float3(-0.2f, 0.93f, -0.1f)), 0.02f);


#ifndef DISABLE_MUZZLEFLASH
	//Add volume to gun
	Volume volume;
	volume.pos = m_gunObject->GetPosition() + m_gunObject->GetRotation().rotateVector(float3(-0.2f, 0.83f, 0.f));
	volume.r = 0.4f;
	volume.rEnd = 0.5f;
	volume.albedo = float3(0.1f);
	volume.emission = float3(1500.f, 200.f, 20.f);
	volume.emissionDensitySubtraction = 0.1f;
	volume.absorption = 1.7f;
	volume.scattering = 1.7f;
	volume.noiseScale = 780.237f;
	volume.faloffStart = 0.2f;
	volume.innerDensityAddition = 0.00f;
	volume.animationSpeed = 738.2f;
	m_renderScene->m_volumeBVH->SpawnFromPool(300.f, volume);
#endif

	Ray r = m_camera.GetPrimaryRay(RENDER_WIDTH * 0.5f, RENDER_HEIGHT * 0.5f, false, seed);
	auto ray = tinybvh::Ray(tinybvh::bvhvec3(r.O.x, r.O.y, r.O.z), tinybvh::bvhvec3(r.D.x, r.D.y, r.D.z));
	m_renderScene->FindNearest(ray);
	if (ray.hit.t < 1.0e30f)
	{
		if (m_renderScene->GetGameObjects()[ray.hit.inst] != nullptr)
		{
			m_renderScene->GetGameObjects()[ray.hit.inst]->OnShot(100.f);
			float3 I = ray.O + ray.D + ray.hit.t;
			AudioManager::GetAudioManager().PlaySoundFile("../assets/Audio/stonehit.wav", false, 0.3f, true,
			                                              I, 0.05f);
		}
	}
}

void Tmpl8::Renderer::PlayerTakeDamage(float _damage)
{
	if (m_playerDied) return;
	m_heliHealth -= _damage;
	if (m_heliHealth <= 0)
	{
		m_playerDied = true;
		AudioManager::GetAudioManager().StopAll();
		AudioManager::GetAudioManager().PlaySoundFile(
			"../assets/Audio/death.wav", false, 0.1f, false);
		AudioManager::GetAudioManager().PlayMusic("../assets/Audio/die.mp3");
	}
}

float3 Renderer::CalculateLighting(const float3& I, const float3* N, float3* _lightDirOut, FloatTexture* _debugScreen)
{
	float rand = SampleBlueNoise();

	float3 lighting = CalculateSkyLight(I, N, _lightDirOut, _debugScreen); // Always calculate skylight to reduce noise

	float prob = 0;

	if (m_renderScene->GetTotalLightCount() <= 0) return lighting;

	if (rand <= 0) rand = EPSILON;

	if (m_renderScene->m_volumeBVH != nullptr)
	{
		prob += static_cast<float>(m_renderScene->m_volumeBVH->m_lightVolumes.size()) / static_cast<float>(m_renderScene
			->
			GetTotalLightCount());
		if (rand <= prob)
		{
			return lighting + CalculateVolumeLight(I, N, _lightDirOut, _debugScreen);
		}
	}
	prob += (static_cast<float>(m_renderScene->GetLightCount()) / static_cast<float>(m_renderScene->
		GetTotalLightCount()));
	if (rand <= prob)
	{
#ifdef USE_SIMD
		return lighting + CalculatePointLight4(I, N, lightDirOut, _debugScreen);
#else

		return lighting + CalculatePointLight(I, N, _lightDirOut, _debugScreen);

#endif
	}
	prob += (static_cast<float>(m_renderScene->GetDirectionalLightCount()) / static_cast<float>(m_renderScene->
		GetTotalLightCount()));

	if (rand <= prob)
	{
		return lighting + CalculateDirectionalLight(I, N, _lightDirOut, _debugScreen);
	}
	return lighting + CalculateSpotLight(I, N, _lightDirOut, _debugScreen);
}

float3 Renderer::CalculatePointLight(const float3& I, const float3* N, float3* _lightDirOut, FloatTexture* _debugScreen)
{
	Light light = m_renderScene->GetLight(seed);
	float3 lightPos = light.pos + float3(light.size.x * ((RandomFloat(seed) - 0.5f) * 2),
	                                     light.size.y * ((RandomFloat(seed) - 0.5f) * 2),
	                                     light.size.z * ((RandomFloat(seed) - 0.5f) * 2));
	float3 L = lightPos - I;
	float lengthL = length(L);
	lengthL = max(lengthL, EPSILON);
	float3 attentuation = 0;
	float lightDist = lengthL - EPSILON;
	float3 normalL = L / lengthL;
	if (_lightDirOut != nullptr)
	{
		_lightDirOut->x = normalL.x;
		_lightDirOut->y = normalL.y;
		_lightDirOut->z = normalL.z;
	}
	float LdotN;
	if (N != nullptr) LdotN = dot(normalL, *N);
	else LdotN = 1.f;
	if (LdotN < 0) return float3(0);

	float3 shadowRayO = I + normalL * EPSILON;
	bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, normalL, lightDist - 2 * EPSILON));
	if (!occluded)
	{
		attentuation = 1 / (lightDist * lightDist);

		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, normalL, lightDist - 2 * EPSILON);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				attentuation *= m_renderScene->m_volumeBVH->GetSimpleContributionThroughVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
			}
		}

		attentuation *= LdotN;
		attentuation *= static_cast<float>(m_renderScene->GetTotalLightCount());
	}
	if (_debugScreen != nullptr)
	{
		//Draw shadow ray
		float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
		int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);
		int x2 = static_cast<int>(lightPos.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y2 = static_cast<int>(lightPos.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x2 = clamp(x2, 0, RENDER_WIDTH - 1);
		y2 = clamp(y2, 0, RENDER_HEIGHT - 1);
		_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x2), static_cast<float>(y2),
		                   color);
	}

	float area = light.size.x * light.size.y * light.size.z;
	if (area == 0) area = 1; // Acount for non area lights

	attentuation *= area;

	float3 lightColor = light.color;
	lightColor *= attentuation;
	return lightColor;
}

float3 Tmpl8::Renderer::CalculatePointLight4(const float3& _I, const float3* _N, float3* /*_lightDirOut*/,
                                             FloatTexture* _debugScreen)
{
	static const __m128 one = _mm_set1_ps(1.f);

	__m128 Ix4 = _mm_set1_ps(_I.x);
	__m128 Iy4 = _mm_set1_ps(_I.y);
	__m128 Iz4 = _mm_set1_ps(_I.z);

	const LightSOA& lights = m_renderScene->GetLightSOA();

	uint light = static_cast<uint>(floor(SampleBlueNoise() * (m_renderScene->GetLightCount() + 1)));
	light /= 4u;
	static const __m128 pointFive = _mm_set1_ps(0.5f);
	static const __m128 two = _mm_set1_ps(2.f);

	__m128 lightPosX4 = lights.x4[light];
	__m128 lightPosY4 = lights.y4[light];
	__m128 lightPosZ4 = lights.z4[light];
	lightPosX4 = _mm_add_ps(lightPosX4, _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(RandomFloat(seed4), pointFive), two),
	                                               lights.sX4[light]));
	lightPosY4 = _mm_add_ps(lightPosY4, _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(RandomFloat(seed4), pointFive), two),
	                                               lights.sY4[light]));
	lightPosZ4 = _mm_add_ps(lightPosZ4, _mm_mul_ps(_mm_mul_ps(_mm_sub_ps(RandomFloat(seed4), pointFive), two),
	                                               lights.sZ4[light]));
	__m128 Lx4 = _mm_sub_ps(lightPosX4, Ix4);
	__m128 Ly4 = _mm_sub_ps(lightPosY4, Iy4);
	__m128 Lz4 = _mm_sub_ps(lightPosZ4, Iz4);

	__m128 sqrLengthL4 = dot(Lx4, Ly4, Lz4, Lx4, Ly4, Lz4);
	union
	{
		__m128 lengthL4;
		float lengthL[4];
	};
	lengthL4 = _mm_sqrt_ps(sqrLengthL4);

	union
	{
		__m128 attentuationR4;
		float attentuationR[4];
	};
	union
	{
		__m128 attentuationG4;
		float attentuationG[4];
	};
	union
	{
		__m128 attentuationB4;
		float attentuationB[4];
	};
	attentuationR4 = _mm_set1_ps(1.f);
	attentuationG4 = _mm_set1_ps(1.f);
	attentuationB4 = _mm_set1_ps(1.f);


	union
	{
		__m128 normalLx4;
		float normalLx[4];
	};
	union
	{
		__m128 normalLy4;
		float normalLy[4];
	};
	union
	{
		__m128 normalLz4;
		float normalLz[4];
	};

	normalLx4 = _mm_div_ps(Lx4, lengthL4);
	normalLy4 = _mm_div_ps(Ly4, lengthL4);
	normalLz4 = _mm_div_ps(Lz4, lengthL4);

	__m128 LdotN4;
	if (_N != nullptr)
	{
		__m128 Nx4 = _mm_set1_ps(_N->x);
		__m128 Ny4 = _mm_set1_ps(_N->y);
		__m128 Nz4 = _mm_set1_ps(_N->z);
		LdotN4 = dot(normalLx4, normalLy4, normalLz4, Nx4, Ny4, Nz4);
	}
	else LdotN4 = _mm_set1_ps(1.f);

	union
	{
		__m128 negativeDot4;
		float negativeDot[4];
	};
	negativeDot4 = _mm_cmplt_ps(LdotN4, _mm_setzero_ps());

	union
	{
		__m128i occludedMask4;
		int occludedMask[4];
	};

	for (int i = 0; i < 4; ++i)
	{
		float3 normalL = float3(normalLx[i], normalLy[i], normalLz[i]);

		float3 shadowRayO = _I + normalL * EPSILON;
		bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, normalL, lengthL[i] - 2 * EPSILON));
		if (occluded)
		{
			occludedMask[i] = -1;
		}
		else occludedMask[i] = 0;
		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, normalL, lengthL[i] - 2 * EPSILON);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				float3 contribute = m_renderScene->m_volumeBVH->GetSimpleContributionThroughVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
				attentuationR[i] = contribute.x;
				attentuationG[i] = contribute.y;
				attentuationB[i] = contribute.z;
			}
		}
		if (_debugScreen != nullptr)
		{
			//Draw shadow ray
			float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
			int x = static_cast<int>(_I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
			int y = static_cast<int>(_I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
			x = clamp(x, 0, RENDER_WIDTH - 1);
			y = clamp(y, 0, RENDER_HEIGHT - 1);
			int x2 = static_cast<int>(_I.x + normalL.x * 100.f * m_debugSettings.scale.x + m_debugSettings.offset.x);
			int y2 = static_cast<int>(_I.z + normalL.z * 100.f * m_debugSettings.scale.y + m_debugSettings.offset.y);
			x2 = clamp(x2, 0, RENDER_WIDTH - 1);
			y2 = clamp(y2, 0, RENDER_HEIGHT - 1);
			_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x2),
			                   static_cast<float>(y2),
			                   color);
		}
	}

	__m128 distanceAttent4 = _mm_rcp_ps(_mm_mul_ps(lengthL4, lengthL4));
	__m128 attentMult = _mm_mul_ps(distanceAttent4, LdotN4);

	__m128 lightCount = _mm_set1_ps(static_cast<float>(m_renderScene->GetTotalLightCount() / 4.f));
	// / 4 because we're sampling 4 lights simultaneously

	attentMult = _mm_mul_ps(attentMult, lightCount);

	__m128 area = _mm_mul_ps(lights.sX4[light], _mm_mul_ps(lights.sY4[light], lights.sZ4[light]));
	area = _mm_max_ps(area, one);

	attentMult = _mm_mul_ps(attentMult, area);

	attentuationR4 = _mm_mul_ps(attentuationR4, attentMult);
	attentuationG4 = _mm_mul_ps(attentuationG4, attentMult);
	attentuationB4 = _mm_mul_ps(attentuationB4, attentMult);

	__m128 mask = _mm_or_ps(_mm_cvtepi32_ps(occludedMask4), negativeDot4);

	attentuationR4 = _mm_andnot_ps(mask, attentuationR4);
	attentuationG4 = _mm_andnot_ps(mask, attentuationG4);
	attentuationB4 = _mm_andnot_ps(mask, attentuationB4);


	float3 lightColor = float3(0);
	for (int i = 0; i < 4; ++i)
	{
		float3 light = float3(lights.r[i], lights.g[i], lights.b[i]);
		light.x *= attentuationR[i];
		light.y *= attentuationG[i];
		light.z *= attentuationB[i];
		lightColor += light;
	}
	return lightColor;
}

float3 Renderer::CalculateSkyLight(const float3& I, const float3* N, float3* _lightDirOut, FloatTexture* _debugScreen)
{
	//uniform sampling
	float3 L;
	float LdotN = 1;
	if (N != nullptr)
	{
		L = BlueCosineRandomUnitVectorHemisphere(*N);
		LdotN = dot(L, *N);
		if (LdotN < 0) return float3(0);
	}
	else
	{
		L = BlueRandomUnitVector();
	}
	if (_lightDirOut != nullptr)
	{
		_lightDirOut->x = L.x;
		_lightDirOut->y = L.y;
		_lightDirOut->z = L.z;
	}
	float3 attentuation = 0;
	float3 shadowRayO = I + L * EPSILON;
	bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, L));
	if (!occluded)
	{
		attentuation = 1;
		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, L);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				attentuation *= m_renderScene->m_volumeBVH->GetSimpleContributionThroughVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
			}
		}
		attentuation *= LdotN;
		//attentuation *= static_cast<float>(renderScene.GetTotalLightCount());
	}
	if (_debugScreen != nullptr)
	{
		//Draw shadow ray
		float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
		int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);

		_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), x + L.x * 5000,
		                   y + L.z * 5000, color);
	}

	if (occluded) return float3(0);

	//attentuation /= LdotN; nullified by *= LdotN
	float3 lightColor = m_skyDome->m_texture->SampleSphere(L);
	lightColor *= attentuation;
	return lightColor;
}

float3 Renderer::CalculateDirectionalLight(const float3& I, const float3* N, float3* _lightDirOut,
                                           FloatTexture* _debugScreen)
{
	DirLight light = m_renderScene->GetDirectionalLight(seed);
	float3 L = -light.direction;
	if (_lightDirOut != nullptr)
	{
		_lightDirOut->x = L.x;
		_lightDirOut->y = L.y;
		_lightDirOut->z = L.z;
	}
	float LdotN;
	if (N != nullptr) LdotN = dot(L, *N);
	else LdotN = 1.f;
	if (LdotN < 0) return float3(0);
	float3 attentuation = 0;
	float3 shadowRayO = I + L * EPSILON;
	bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, L));
	if (!occluded)
	{
		attentuation = 1;
		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, L);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				attentuation *= m_renderScene->m_volumeBVH->GetSimpleContributionThroughVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
			}
		}
		attentuation *= LdotN;
		attentuation *= static_cast<float>(m_renderScene->GetTotalLightCount());
	}
	if (_debugScreen != nullptr)
	{
		//Draw shadow ray
		float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
		int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);

		_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), x + L.x * 5000,
		                   static_cast<float>(y) + (L.z * 5000), color);
	}
	float3 lightColor = light.color;
	lightColor *= attentuation;
	return lightColor;
}

float3 Renderer::CalculateSpotLight(const float3& I, const float3* N, float3* _lightDirOut, FloatTexture* _debugScreen)
{
	float3 lightColor(0);

	const SpotLight& spotLight = m_renderScene->GetSpotLight(seed);
	float3 L = spotLight.pos - I;
	float3 attentuation = 0;
	float lightDist = length(L) - EPSILON;
	float3 normalL = normalize(L);
	if (_lightDirOut != nullptr)
	{
		_lightDirOut->x = normalL.x;
		_lightDirOut->y = normalL.y;
		_lightDirOut->z = normalL.z;
	}
	float LdotN;
	if (N != nullptr) LdotN = dot(normalL, *N);
	else LdotN = 1.f;
	if (LdotN < 0) return float3(0);

	float3 shadowRayO = I + normalL * EPSILON;
	bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, normalL, lightDist - (EPSILON * 2)));
	if (!occluded)
	{
		attentuation = 1 / (lightDist * lightDist * 4 * PI);
		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, normalL, lightDist - 2 * EPSILON);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				attentuation *= m_renderScene->m_volumeBVH->GetSimpleContributionThroughVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
			}
		}
		attentuation *= LdotN;

		float spotDot = -dot(normalL, spotLight.direction);
		if (spotDot < spotLight.outerCutoff) attentuation = 0;

		attentuation *= (spotDot - spotLight.outerCutoff) / (spotLight.innerCutoff - spotLight.outerCutoff);
		attentuation *= static_cast<float>(m_renderScene->GetTotalLightCount());

		if (spotLight.hasTexture)
		{
			float scale = spotLight.scale;
			static const auto worldUp = float3(0, 1, 0);

			float3 right;
			if (spotLight.direction.x == worldUp.x && spotLight.direction.y == worldUp.y && spotLight.direction.z ==
				worldUp.z)
				right = float3(1, 0, 0);
			else right = normalize(cross(spotLight.direction, worldUp));
			float3 up = normalize(cross(right, spotLight.direction));

			float u = (dot(-normalL, right) + 1) * 0.5f;
			float v = 1 - ((dot(-normalL, up) + 1) * 0.5f);
			u = u * scale + ((1 - scale) * 0.5f);
			v = v * scale + ((1 - scale) * 0.5f);

			if (u >= 1 || u < 0 || v >= 1 || v < 0)
			{
				if (_debugScreen == nullptr)
					return float3(0); // no light outside texture
				lightColor = float3(0);
			}
			else
			{
				const FloatTexture& texture = m_renderScene->GetTextureManager().GetTexture(spotLight.texture);
				const auto resolution = int2(texture.m_resolution.x, texture.m_resolution.y);
				int ix = static_cast<int>((u) * (resolution.x)), iy = static_cast<int>((v) * (resolution.y));
				float3 p = texture.m_pixels[(ix) + (iy) * resolution.x];
				lightColor = p * spotLight.intensity;
			}
		}
		else
		{
			lightColor = spotLight.color;
		}
		lightColor *= attentuation;
	}
	if (_debugScreen != nullptr)
	{
		//Draw shadow ray
		float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
		int x = static_cast<int>(I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);
		int x2 = static_cast<int>(spotLight.pos.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y2 = static_cast<int>(spotLight.pos.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x2 = clamp(x2, 0, RENDER_WIDTH - 1);
		y2 = clamp(y2, 0, RENDER_HEIGHT - 1);
		_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x2), static_cast<float>(y2),
		                   color);
	}
	return lightColor;
}

float3 Tmpl8::Renderer::CalculateVolumeLight(const float3& _I, const float3* _N, float3* _lightDirOut,
                                             FloatTexture* _debugScreen)
{
	float4 light = m_renderScene->GetVolumeLight(seed);
	float3 lightPos = float3(light) + RandomUnitVector(seed) * light.w;
	float3 L = lightPos - _I;
	float lengthL = length(L);

	float3 attentuation = 0;
	float lightDist = lengthL - EPSILON;
	float3 normalL = L / lengthL;
	if (_lightDirOut != nullptr)
	{
		_lightDirOut->x = normalL.x;
		_lightDirOut->y = normalL.y;
		_lightDirOut->z = normalL.z;
	}
	float LdotN;
	if (_N != nullptr) LdotN = dot(normalL, *_N);
	else LdotN = 1.f;
	if (LdotN < 0) return float3(0);

	float3 shadowRayO = _I + normalL * EPSILON;
	float3 lightColor = float3(0);
	bool occluded = m_renderScene->IsOccluded(tinybvh::Ray(shadowRayO, normalL, lightDist - 2 * EPSILON));
	if (!occluded)
	{
		attentuation = 1 / (lightDist * lightDist);

		if (m_renderScene->m_volumeBVH != nullptr)
		{
			//TODO: currently wont work correctly if theres multiple volumes in the path but that should be rare.
			auto fogRay = tinybvh::Ray(shadowRayO, normalL, lightDist - 2 * EPSILON);
			m_renderScene->m_volumeBVH->bvh.Intersect(fogRay);
			if (fogRay.hit.t < 1.0E20)
			{
				lightColor = m_renderScene->m_volumeBVH->GetSimpleEmissionVolume(
					fogRay.hit.prim, fogRay.O, fogRay.D, seed);
			}
			else { return float3(0); }
		}
		else { return float3(0); }

		attentuation *= LdotN;
		attentuation *= static_cast<float>(m_renderScene->GetTotalLightCount());
		attentuation *= 4.1888f * light.w * light.w * light.w; // Volume of a sphere
	}
	if (_debugScreen != nullptr)
	{
		//Draw shadow ray
		float4 color = occluded ? float4(0, 0, 0.3f, 1.f) : float4(0.2f, 0.2f, 1.f, 1.f);
		int x = static_cast<int>(_I.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y = static_cast<int>(_I.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x = clamp(x, 0, RENDER_WIDTH - 1);
		y = clamp(y, 0, RENDER_HEIGHT - 1);
		int x2 = static_cast<int>(lightPos.x * m_debugSettings.scale.x + m_debugSettings.offset.x);
		int y2 = static_cast<int>(lightPos.z * m_debugSettings.scale.y + m_debugSettings.offset.y);
		x2 = clamp(x2, 0, RENDER_WIDTH - 1);
		y2 = clamp(y2, 0, RENDER_HEIGHT - 1);
		_debugScreen->Line(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x2), static_cast<float>(y2),
		                   color);
	}
	return lightColor * attentuation;
}

float Tmpl8::Renderer::SampleBlueNoise()
{
	if (m_sampleBlueNoise)
	{
		uint& timesSampled = blueNoiseSamples[rayScreenPos.x + rayScreenPos.y * RENDER_WIDTH];
		timesSampled = timesSampled + 1;
		uint texture = timesSampled;
		if (texture >= static_cast<uint>(m_noiseTextures.size()))
		{
			return RandomFloat(seed);
		}
		float2 rayPosF = float2(static_cast<float>(rayScreenPos.x), static_cast<float>(rayScreenPos.y)) * 1.f;
		int2 rayPos = int2(static_cast<int>(rayPosF.x), static_cast<int>(rayPosF.y));
		rayPos.x = rayPos.x % (m_noiseTextures[texture]->m_resolution.x);
		rayPos.y = rayPos.y % (m_noiseTextures[texture]->m_resolution.y);
		float3 sample = m_noiseTextures[texture]->m_pixels[rayPos.x + rayPos.y * (m_noiseTextures[texture]->m_resolution
			.
			x)];

		return sample.x;
	}
	return RandomFloat(seed);
}


float3 Tmpl8::Renderer::BlueCosineRandomUnitVectorHemisphere(float3 _normal)
{
	float3 dir = _normal + BlueRandomUnitVector();
	float len2 = length2(dir);
	if (len2 < 0.0001f)
	{
		return float3(0, 0, 0);
	}
	return dir / sqrt(len2);
}

float3 Tmpl8::Renderer::BlueRandomUnitVector()
{
	auto vec = float3(SampleBlueNoise() - 0.5f, SampleBlueNoise() - 0.5f, SampleBlueNoise() - 0.5f);
	float len2 = length2(vec);
	if (len2 < 0.0001f)
	{
		return float3(0, 0, 0);
	}
	return vec / sqrt(len2);
}

void Tmpl8::Renderer::SetDifficulty(Difficulty _diff)
{
	switch (_diff)
	{
	case Difficulty::GameReviewer:
		m_difficulty = Difficulty::GameReviewer;
		m_heliHealth = 100.f;
		m_heliMaxHealth = 100.f;
		for (uint i = 0; i < m_renderScene->GetObjectAmount(); ++i)
		{
			GameObject* obj = m_renderScene->GetGameObjects()[i];

			if (obj != nullptr)
			{
				EnemyObject* eObj = dynamic_cast<EnemyObject*>(obj);
				if (eObj != nullptr)
				{
					eObj->SetAttackSpeed(6000.f, 4000.f);
					eObj->SetMaxRange(17.f);
				}
			}
		}
		break;
	case Difficulty::Normal:
		m_difficulty = Difficulty::Normal;
		m_heliHealth = 50.f;
		m_heliMaxHealth = 50.f;
		for (uint i = 0; i < m_renderScene->GetObjectAmount(); ++i)
		{
			GameObject* obj = m_renderScene->GetGameObjects()[i];

			if (obj != nullptr)
			{
				EnemyObject* eObj = dynamic_cast<EnemyObject*>(obj);
				if (eObj != nullptr)
				{
					eObj->SetAttackSpeed(
						5000.f, 1000.f);
					eObj->SetMaxRange(20.f);
				}
			}
		}
		break;
	case Difficulty::Hard:
		m_difficulty = Difficulty::Hard;
		m_heliHealth = 40.f;
		m_heliMaxHealth = 40.f;
		for (uint i = 0; i < m_renderScene->GetObjectAmount(); ++i)
		{
			GameObject* obj = m_renderScene->GetGameObjects()[i];

			if (obj != nullptr)
			{
				EnemyObject* eObj = dynamic_cast<EnemyObject*>(obj);
				if (eObj != nullptr)
				{
					eObj->SetAttackSpeed(3000.f, 500.f);
					eObj->SetMaxRange(25.f);
				}
			}
		}
		break;
	case Difficulty::EldenRing:
		m_difficulty = Difficulty::EldenRing;
		m_heliHealth = 40.f;
		m_heliMaxHealth = 40.f;
		for (uint i = 0; i < m_renderScene->GetObjectAmount(); ++i)
		{
			GameObject* obj = m_renderScene->GetGameObjects()[i];

			if (obj != nullptr)
			{
				EnemyObject* eObj = dynamic_cast<EnemyObject*>(obj);
				if (eObj != nullptr)
				{
					eObj->SetAttackSpeed(1000.f, 200.f);
					eObj->SetMaxRange(27.f);
				}
			}
		}
		break;
	}
}
