#pragma once
#include "MarchingVolumeBVH.h"
#include "Model.hpp"
#include "TextureManager.h"
#include "MaterialManager.h"

namespace Tmpl8
{
	class Camera;
}

class GameObject;
class MarchingVolumeBVH;
class SphereBVH;
class CubeBVH;
struct Sphere;

struct Light
{
	float3 pos;
	float3 color;
	float3 size;
};

struct LightSOA
{
	union
	{
		__m128 x4[((MAX_LIGHTS) / 4) + 1];
		float x[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 y4[((MAX_LIGHTS) / 4) + 1];
		float y[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 z4[((MAX_LIGHTS) / 4) + 1];
		float z[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 r4[((MAX_LIGHTS) / 4) + 1];
		float r[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 g4[((MAX_LIGHTS) / 4) + 1];
		float g[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 b4[((MAX_LIGHTS) / 4) + 1];
		float b[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 sX4[((MAX_LIGHTS) / 4) + 1];
		float sX[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 sY4[((MAX_LIGHTS) / 4) + 1];
		float sY[(MAX_LIGHTS + 3)];
	};

	union
	{
		__m128 sZ4[((MAX_LIGHTS) / 4) + 1];
		float sZ[(MAX_LIGHTS + 3)];
	};
};

struct DirLight
{
	float3 direction;
	float3 color;
};

struct SpotLight
{
	float3 pos;
	float3 direction;

	union
	{
		float3 color;

		struct
		{
			int texture;
			float scale;
			float intensity;
		};
	};

	float innerCutoff;
	float outerCutoff;
	bool hasTexture;
};

enum class PrimitiveType
{
	Triangles,
	Sphere,
	Cube
};

struct TriangleData
{
	VertexData *vert0, *vert1, *vert2;
};

class RenderScene
{
public:
	RenderScene(Camera& _sceneCamera);
	~RenderScene();
	void Tick(float _deltaTime);
	int AddModel(const Model& _model);
	int AddSphere(float3 _pos, float _r, int _material, int _texture = -1);
	int AddSphere(const Sphere* _spheres, uint _amount);
	int AddCube(const mat4& _transform, float3 _halfSize, int _material, int _texture = -1);
	void AddLight(const Light& _light);
	void SetupLightSOA();
	void AddDirLight(const DirLight& _light);
	void AddSpotLight(const SpotLight& _light);
	void AddSpotLight(float3 _pos, float3 _direction, float3 _color, float _innerCutoff, float _outerCutoff);
	void AddSpotLight(float3 _pos, float3 _direction, int _texture, float _scale, float _intensity, float _innerCutoff,
	                  float _outerCutoff);
	void RemoveObject(uint _instance);
	void BuildTLAS();
	tinybvh::BLASInstance& GetBLAS(const int _id) { return m_instanceList[_id]; }
	void FindNearest(tinybvh::Ray& _ray) const;
	bool IsOccluded(const tinybvh::Ray& _ray) const;
	const Material& GetMaterial(uint _instance, uint _primitiveIndex, float2 _triangleUv) const;
	float3 SampleTextureOnTri(float2 _uv, const TriangleData& _hitTriangle, const FloatTexture& _texture,
	                          float _mip = 0.f) const;
	float4 RenderScene::SampleTextureOnTriXYZW(float2 _uv, const TriangleData& _hitTriangle,
	                                           const FloatTexture& _texture,
	                                           float _mip) const;
	float3 GetAlbedo(uint _instance, uint _primitiveIndex, float2 _triangleUv, bool _mipMap,
	                 const float3& _normal, const float3& _rayDir, float _rayLength, float _vFov, float _lodBias) const;
	float3 GetEmission(uint _instance, uint _primitiveIndex, float2 _triangleUv) const;
	float GetReflectivity(uint _instance, uint _index, float2 _uv, bool _mipMap,
	                      const float3& _normal, const float3& _rayDir, float _rayLength, float _vFov,
	                      float _lodBias) const;
	float GetRoughness(uint _instance, uint _index, float2 _uv, bool _mipMap,
	                   const float3& _normal, const float3& _rayDir, float _rayLength, float _vFov,
	                   float _lodBias) const;
	//float GetTransmission(uint instance, uint primitiveIndex, float2 triangleUV);
	float3 GetNormal(uint _instance, uint _primitiveIndex, float2 _triangleUv, float3 _normalRayDirection, float3 _I,
	                 bool& _isFrontFace, bool _mipMap, float _lodBias, float _vFOV, float _rayLength);

	const uint GetTotalLightCount()
	{
		int volumeLights = m_volumeBVH != nullptr ? static_cast<int>(m_volumeBVH->m_lightVolumes.size()) : 0;
		return m_numberOfLights + m_numberOfSpotLights + m_numberOfDirectionalLights + volumeLights;
	}

	const uint GetLightCount() const { return m_numberOfLights; }
	const uint GetSpotLightCount() const { return m_numberOfSpotLights; }
	const uint GetDirectionalLightCount() const { return m_numberOfDirectionalLights; }
	const Light& GetLight(uint _seed) const;
	const LightSOA& RenderScene::GetLightSOA() const;
	const DirLight& GetDirectionalLight(uint _seed) const;
	const SpotLight& GetSpotLight(uint _seed) const;
	const float4& GetVolumeLight(uint _seed) const;
	const VertexData& GetVertexData(uint _instance, uint _triangleIndex) const;
	const TriangleData GetTriangleData(uint _instance, uint _triangleIndex) const;
	TextureManager& GetTextureManager() { return m_textureManager; }
	MaterialManager& GetMaterialManager() { return m_materialManager; }
	GameObject** GetGameObjects() { return m_gameObjects; }
	uint GetObjectAmount() const { return m_instances - 1; }
	SphereBVH* GetSphereData(const uint _inst) const { return m_sphereData[_inst]; }
private:
	tinybvh::BLASInstance m_instanceList[MAX_BLAS]{};
	Light m_lights[MAX_LIGHTS]{};
	DirLight m_dirLights[MAX_LIGHTS]{};
	SpotLight m_spotLights[MAX_LIGHTS]{};
	PrimitiveType m_typeList[MAX_BLAS]{};
	tinybvh::bvhvec4* m_bvhVerts[MAX_BLAS]{};
	tinybvh::BVHBase* m_bvhList[MAX_BLAS]{};
	VertexData* m_vertexData[MAX_BLAS]{};
	SphereBVH* m_sphereData[MAX_BLAS]{};
	CubeBVH* m_cubeData[MAX_BLAS]{};
	uint* m_indices[MAX_BLAS]{};
	GameObject* m_gameObjects[MAX_BLAS]{};
	Camera* m_sceneCamera{nullptr};

public:
	MarchingVolumeBVH* m_volumeBVH{nullptr};

private:
	LightSOA m_lightSOA;

	tinybvh::BVH m_TLAS;
	TextureManager m_textureManager;
	MaterialManager m_materialManager;
	uint m_instances = 0;

	uint m_numberOfLights = 0;
	uint m_numberOfSpotLights = 0;
	uint m_numberOfDirectionalLights = 0;
};
