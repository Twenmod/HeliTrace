#include "precomp.h"
#include "RenderScene.h"

#include "SphereBVH.h"
#include "CubeBVH.h"
#include "GameObject.h"
#include "EnemyObject.h"
#include "BarrelObject.h"
#include "MarchingVolumeBVH.h"
#include "ScoreManager.h"

RenderScene::RenderScene(Camera& _sceneCamera)
{
	m_sceneCamera = &_sceneCamera;
	for (int i = 0; i < MAX_BLAS; ++i)
	{
		m_gameObjects[i] = nullptr;
	}
}

RenderScene::~RenderScene()
{
	delete m_volumeBVH;
	for (uint i = 0; i < m_instances; ++i)
	{
		delete[] m_vertexData[i];
		delete[] m_bvhVerts[i];
		delete[] m_cubeData[i];
		delete[] m_gameObjects[i];
		delete[] m_indices[i];
	}
}

void RenderScene::Tick(float _deltaTime)
{
	if (m_volumeBVH != nullptr) m_volumeBVH->Tick(_deltaTime);
	for (uint i = 0; i < m_instances; ++i)
	{
		if (m_gameObjects[i] == nullptr) continue;
		m_gameObjects[i]->Tick(_deltaTime);
	}
	BuildTLAS();
}

int RenderScene::AddModel(const Model& _model)
{
	bool fullyAnimated = true;
	for (const Mesh* mesh : _model.m_meshes)
	{
		if (!mesh->m_hasAnimation) fullyAnimated = false;
	}
	if (fullyAnimated)
	{
		for (const Mesh* mesh : _model.m_meshes)
		{
			m_vertexData[m_instances] = new VertexData[mesh->m_verticesData.size()];
			m_bvhVerts[m_instances] = new tinybvh::bvhvec4[mesh->m_verticesData.size()];
			m_indices[m_instances] = new uint[mesh->m_indices.size()];

			for (int i = 0; i < static_cast<int>(mesh->m_vertices.size()); i++)
			{
				m_vertexData[m_instances][i] = mesh->m_verticesData[i];
				m_bvhVerts[m_instances][i] = reinterpret_cast<const tinybvh::bvhvec4&>(mesh->m_vertices[i].
					position);
			}
			for (int i = 0; i < static_cast<int>(mesh->m_indices.size()); i++)
			{
				m_indices[m_instances][i] = mesh->m_indices[i];
			}

			uint triangles = static_cast<int>(mesh->m_indices.size()) / 3;
			//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
				auto newBVH = new tinybvh::BVH();
#else
			auto newBVH = new tinybvh::BVH8_CPU();
#endif

			newBVH->BuildHQ(m_bvhVerts[m_instances],
			                m_indices[m_instances], triangles);
			m_bvhList[m_instances] = newBVH;
			m_typeList[m_instances] = PrimitiveType::Triangles;

			m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

			//Create gameobject
			GameObject* object = new GameObject(m_instances, m_instanceList[m_instances], *this, mesh->m_name,
			                                    mesh->m_animation);
			m_gameObjects[m_instances] = object;
			++m_instances;
		}
		return m_instances - 1;
	}

	int totalVerts = 0;
	int totalIndices = 0;
	for (const Mesh* mesh : _model.m_meshes)
	{
		totalVerts += static_cast<int>(mesh->m_vertices.size());
		totalIndices += static_cast<int>(mesh->m_indices.size());
	}

	//Load custom objects i.e enemies barrels
	uint meshSize = static_cast<uint>(_model.m_meshes.size());
	for (uint i = 0; i < meshSize - 1; ++i)
	{
		const Mesh* mesh = _model.m_meshes[i];
		//
		if (mesh->m_name.rfind("enemy", 0) == 0)
		{
			int instance = m_instances++;
			std::vector<const Mesh*> splitMeshes; // meshes are split on material so need to recombine them
			std::string enemyName = mesh->m_name.substr(0, 9);
			int tVerts = 0;
			int tIndices = 0;
			for (uint j = i; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];
				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					tVerts += static_cast<int>(mesh_->m_vertices.size());
					tIndices += static_cast<int>(mesh_->m_indices.size());
				}
				else break;
			}
			m_vertexData[instance] = new VertexData[tVerts];
			m_bvhVerts[instance] = new tinybvh::bvhvec4[tVerts];
			m_indices[instance] = new uint[tIndices];
			uint vertPos = 0;
			uint indPos = 0;
			i--;
			for (uint j = i + 1; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];
				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					int indicesOffset = vertPos;
					for (int k = 0; k < mesh_->m_vertices.size(); k++)
					{
						m_vertexData[instance][vertPos] = mesh_->m_verticesData[k];
						m_bvhVerts[instance][vertPos] = reinterpret_cast<const tinybvh::bvhvec4&>(mesh_->m_vertices[
							k].position);
						vertPos++;
					}
					for (int k = 0; k < mesh_->m_indices.size(); k++)
					{
						m_indices[instance][indPos] = mesh_->m_indices[k] + indicesOffset;
						indPos++;
					}
					i++; // make sure we do not loop over this again
				}
				else break;
			}

			uint triangles = tIndices / 3;
			//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
			auto newBVH = new tinybvh::BVH();
#else
			auto newBVH = new tinybvh::BVH8_CPU();
#endif

			newBVH->BuildHQ(m_bvhVerts[instance],
			                m_indices[instance], triangles);
			m_bvhList[instance] = newBVH;
			m_typeList[instance] = PrimitiveType::Triangles;

			m_instanceList[instance] = tinybvh::BLASInstance(instance);

			//Create gameobject
			GameObject* object = new EnemyObject(*m_sceneCamera, mesh->m_transformation, GameObject(
				                                     instance, m_instanceList[instance], *this,
				                                     mesh->m_name));
			m_gameObjects[instance] = object;
			ScoreManager::GetScoreManager()->AddTotalEnemies();
		}
		else if (mesh->m_name.rfind("barrel", 0) == 0)
		{
			std::vector<const Mesh*> splitMeshes; // meshes are split on material so need to recombine them
			std::string enemyName = mesh->m_name.substr(0, 10);
			int tVerts = 0;
			int tIndices = 0;
			for (uint j = i; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];
				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					tVerts += static_cast<int>(mesh_->m_vertices.size());
					tIndices += static_cast<int>(mesh_->m_indices.size());
				}
				else break;
			}
			m_vertexData[m_instances] = new VertexData[tVerts];
			m_bvhVerts[m_instances] = new tinybvh::bvhvec4[tVerts];
			m_indices[m_instances] = new uint[tIndices];
			uint vertPos = 0;
			uint indPos = 0;
			i--;
			for (uint j = i + 1; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];

				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					int indicesOffset = vertPos;
					for (int k = 0; k < mesh_->m_vertices.size(); k++)
					{
						m_vertexData[m_instances][vertPos] = mesh_->m_verticesData[k];
						m_bvhVerts[m_instances][vertPos] = reinterpret_cast<const tinybvh::bvhvec4&>(mesh_->m_vertices[
							k].position);
						vertPos++;
					}
					for (int k = 0; k < mesh_->m_indices.size(); k++)
					{
						m_indices[m_instances][indPos] = mesh_->m_indices[k] + indicesOffset;
						indPos++;
					}
					i++; // make sure we do not loop over this again
				}
				else break;
			}

			uint triangles = tIndices / 3;
			//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
			auto newBVH = new tinybvh::BVH();
#else
			auto newBVH = new tinybvh::BVH8_CPU();
#endif

			newBVH->BuildHQ(m_bvhVerts[m_instances],
			                m_indices[m_instances], triangles);
			m_bvhList[m_instances] = newBVH;
			m_typeList[m_instances] = PrimitiveType::Triangles;

			m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

			//Create gameobject
			if (m_volumeBVH == nullptr)
			{
				printf("WARNING: CANNOT SPAWN BARREL WITHOUT VOLUMEBVH PRESENT\n");
			}
			else
			{
				GameObject* object = new BarrelObject(*m_volumeBVH, GameObject(m_instances, m_instanceList[m_instances],
				                                                               *this,
				                                                               mesh->m_name));
				object->SetTransform(mesh->m_transformation);
				m_gameObjects[m_instances] = object;
			}
			++m_instances;
		}
		else if (mesh->m_name.rfind("gun", 0) == 0)
		{
			std::vector<const Mesh*> splitMeshes; // meshes are split on material so need to recombine them
			std::string enemyName = mesh->m_name.substr(0, 7);
			int tVerts = 0;
			int tIndices = 0;
			for (uint j = i; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];
				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					tVerts += static_cast<int>(mesh_->m_vertices.size());
					tIndices += static_cast<int>(mesh_->m_indices.size());
				}
				else break;
			}
			m_vertexData[m_instances] = new VertexData[tVerts];
			m_bvhVerts[m_instances] = new tinybvh::bvhvec4[tVerts];
			m_indices[m_instances] = new uint[tIndices];
			uint vertPos = 0;
			uint indPos = 0;
			i--;
			for (uint j = i + 1; j < meshSize; ++j)
			{
				const Mesh* mesh_ = _model.m_meshes[j];
				if (mesh_->m_name.rfind(enemyName, 0) == 0)
				{
					int indicesOffset = vertPos;
					for (int k = 0; k < mesh_->m_vertices.size(); k++)
					{
						m_vertexData[m_instances][vertPos] = mesh_->m_verticesData[k];
						m_bvhVerts[m_instances][vertPos] = reinterpret_cast<const tinybvh::bvhvec4&>(mesh_->m_vertices[
							k].position);
						vertPos++;
					}
					for (int k = 0; k < mesh_->m_indices.size(); k++)
					{
						m_indices[m_instances][indPos] = mesh_->m_indices[k] + indicesOffset;
						indPos++;
					}
					i++; // make sure we do not loop over this again
				}
				else break;
			}

			uint triangles = tIndices / 3;
			//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
				auto newBVH = new tinybvh::BVH();
#else
			auto newBVH = new tinybvh::BVH8_CPU();
#endif

			newBVH->BuildHQ(m_bvhVerts[m_instances],
			                m_indices[m_instances], triangles);
			m_bvhList[m_instances] = newBVH;
			m_typeList[m_instances] = PrimitiveType::Triangles;

			m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

			//Create gameobject
			GameObject* object = new GameObject(m_instances, m_instanceList[m_instances], *this,
			                                    mesh->m_name);
			object->SetTransform(mesh->m_transformation);
			m_gameObjects[m_instances] = object;
			++m_instances;
		}
	}


	uint baseInstance = m_instances++;
	//Load mesh data
	m_vertexData[baseInstance] = new VertexData[totalVerts];
	m_bvhVerts[baseInstance] = new tinybvh::bvhvec4[totalVerts];
	m_indices[baseInstance] = new uint[totalIndices];
	int vert = 0;
	int indic = 0;
	for (const Mesh* mesh : _model.m_meshes)
	{
		//
		if (mesh->m_name.rfind("enemy", 0) == 0) continue;
		if (mesh->m_name.rfind("barrel", 0) == 0) continue;
		if (mesh->m_name.rfind("gun", 0) == 0) continue;
		if (mesh->m_hasAnimation) // If the mesh has animation store it as its own blas
		{
			m_vertexData[m_instances] = new VertexData[mesh->m_verticesData.size()];
			m_bvhVerts[m_instances] = new tinybvh::bvhvec4[mesh->m_verticesData.size()];
			m_indices[m_instances] = new uint[mesh->m_indices.size()];

			for (int i = 0; i < mesh->m_vertices.size(); i++)
			{
				m_vertexData[m_instances][i] = mesh->m_verticesData[i];
				m_bvhVerts[m_instances][i] = reinterpret_cast<const tinybvh::bvhvec4&>(mesh->m_vertices[i].
					position);
			}
			for (int i = 0; i < mesh->m_indices.size(); i++)
			{
				m_indices[m_instances][i] = mesh->m_indices[i];
			}

			uint triangles = static_cast<uint>(mesh->m_indices.size()) / 3;
			//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
			auto newBVH = new tinybvh::BVH();
#else
			auto newBVH = new tinybvh::BVH8_CPU();
#endif

			newBVH->BuildHQ(m_bvhVerts[m_instances],
			                m_indices[m_instances], triangles);
			m_bvhList[m_instances] = newBVH;
			m_typeList[m_instances] = PrimitiveType::Triangles;

			m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

			//Create gameobject
			GameObject* object = new GameObject(m_instances, m_instanceList[m_instances], *this, mesh->m_name,
			                                    mesh->m_animation);
			object->SetTransform(mesh->m_transformation);
			m_gameObjects[m_instances] = object;
			++m_instances;
		}
		else
		{
			int indicesOffset = vert; // offset since meshes store indices locally
			for (int i = 0; i < mesh->m_vertices.size(); i++)
			{
				VertexData data = mesh->m_verticesData[i];
				data.normal = data.normal * mesh->m_transformation.Inverted().Transposed();
				data.bitangent = data.bitangent * mesh->m_transformation.Inverted().Transposed();
				data.tangent = data.tangent * mesh->m_transformation.Inverted().Transposed();
				m_vertexData[baseInstance][vert] = data;
				m_bvhVerts[baseInstance][vert] = (mesh->m_vertices[i].
					position * mesh->m_transformation);
				vert++;
			}
			for (int i = 0; i < mesh->m_indices.size(); i++)
			{
				m_indices[baseInstance][indic++] = mesh->m_indices[i] + indicesOffset;
			}
		}
	}
	uint triangles = indic / 3;


	//Create a bvh for the mesh
#ifdef MAXIMUM_COMPATABILITY
	auto newBVH = new tinybvh::BVH();
#else
	auto newBVH = new tinybvh::BVH8_CPU();
#endif

	newBVH->BuildHQ(m_bvhVerts[baseInstance],
	                m_indices[baseInstance], triangles);
	m_bvhList[baseInstance] = newBVH;
	m_typeList[baseInstance] = PrimitiveType::Triangles;

	m_instanceList[baseInstance] = tinybvh::BLASInstance(baseInstance);
	GameObject* object = new GameObject(baseInstance, m_instanceList[baseInstance], *this, _model.m_meshes[0]->m_name);
	m_gameObjects[baseInstance] = object;

	return baseInstance;
}

int RenderScene::AddSphere(const float3 _pos, const float _r, const int _material, const int _texture)
{
	m_sphereData[m_instances] = new SphereBVH(_pos, _r, _material, _texture);

	m_bvhList[m_instances] = &m_sphereData[m_instances]->m_bvh;
	m_typeList[m_instances] = PrimitiveType::Sphere;
	m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);
	return m_instances++;
}

int RenderScene::AddSphere(const Sphere* _spheres, const uint _amount)
{
	m_sphereData[m_instances] = new SphereBVH(_spheres, _amount);

	m_bvhList[m_instances] = &m_sphereData[m_instances]->m_bvh;
	m_typeList[m_instances] = PrimitiveType::Sphere;
	m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

	return m_instances++;
}

int RenderScene::AddCube(const mat4& _transform, const float3 _halfSize, const int _material, const int _texture)
{
	m_cubeData[m_instances] = new CubeBVH(_transform, _halfSize, _material, _texture);

	m_bvhList[m_instances] = &m_cubeData[m_instances]->m_bvh;
	m_typeList[m_instances] = PrimitiveType::Cube;
	m_instanceList[m_instances] = tinybvh::BLASInstance(m_instances);

	return m_instances++;
}


void RenderScene::AddLight(const Light& _light)
{
	m_lights[m_numberOfLights] = _light;

	m_numberOfLights++;
}

void RenderScene::SetupLightSOA()
{
	for (uint i = 0; i < m_numberOfLights; ++i)
	{
		const Light& light = m_lights[i];
		m_lightSOA.x[i] = light.pos.x;
		m_lightSOA.y[i] = light.pos.y;
		m_lightSOA.z[i] = light.pos.z;
		m_lightSOA.r[i] = light.color.x;
		m_lightSOA.g[i] = light.color.y;
		m_lightSOA.b[i] = light.color.z;
		m_lightSOA.sX[i] = light.size.x;
		m_lightSOA.sY[i] = light.size.y;
		m_lightSOA.sZ[i] = light.size.z;
	}
	//Repeat remainder to end of soa to allow sampling correctly
	if (m_numberOfLights < 4) // Less then 4 is kinda wrong which causes more noise but it does not matter that much
	{
		const Light& light = m_lights[0];
		for (uint i = 0; i < 4 - m_numberOfLights; ++i)
		{
			m_lightSOA.x[m_numberOfLights + i] = light.pos.x;
			m_lightSOA.y[m_numberOfLights + i] = light.pos.y;
			m_lightSOA.z[m_numberOfLights + i] = light.pos.z;
			m_lightSOA.r[m_numberOfLights + i] = light.color.x;
			m_lightSOA.g[m_numberOfLights + i] = light.color.y;
			m_lightSOA.b[m_numberOfLights + i] = light.color.z;
			m_lightSOA.sX[m_numberOfLights + i] = light.size.x;
			m_lightSOA.sY[m_numberOfLights + i] = light.size.y;
			m_lightSOA.sZ[m_numberOfLights + i] = light.size.z;
		}
	}
	else
	{
		int remainder = m_numberOfLights % 4;
		for (int i = 0; i < remainder; ++i)
		{
			const Light& light = m_lights[i];
			m_lightSOA.x[m_numberOfLights + i] = light.pos.x;
			m_lightSOA.y[m_numberOfLights + i] = light.pos.y;
			m_lightSOA.z[m_numberOfLights + i] = light.pos.z;
			m_lightSOA.r[m_numberOfLights + i] = light.color.x;
			m_lightSOA.g[m_numberOfLights + i] = light.color.y;
			m_lightSOA.b[m_numberOfLights + i] = light.color.z;
			m_lightSOA.sX[m_numberOfLights + i] = light.size.x;
			m_lightSOA.sY[m_numberOfLights + i] = light.size.y;
			m_lightSOA.sZ[m_numberOfLights + i] = light.size.z;
		}
	}
}

void RenderScene::AddDirLight(const DirLight& _light)
{
	m_dirLights[m_numberOfDirectionalLights++] = _light;
}

void RenderScene::AddSpotLight(const SpotLight& _light)
{
	m_spotLights[m_numberOfSpotLights++] = _light;
}

void RenderScene::AddSpotLight(const float3 _pos, const float3 _direction, const float3 _color,
                               const float _innerCutoff, const float _outerCutoff)
{
	SpotLight light;
	light.pos = _pos;
	light.direction = _direction;
	light.color = _color;
	light.hasTexture = false;
	light.innerCutoff = cos(DegToRad(_innerCutoff));
	light.outerCutoff = cos(DegToRad(_outerCutoff));
	m_spotLights[m_numberOfSpotLights++] = light;
}

void RenderScene::AddSpotLight(const float3 _pos, const float3
                               _direction, const int _texture, const float _scale, const float _intensity,
                               const float _innerCutoff, const float _outerCutoff)
{
	SpotLight light;
	light.pos = _pos;
	light.direction = _direction;
	light.texture = _texture;
	light.scale = _scale;
	light.intensity = _intensity;
	light.hasTexture = true;
	light.innerCutoff = cos(DegToRad(_innerCutoff));
	light.outerCutoff = cos(DegToRad(_outerCutoff));
	m_spotLights[m_numberOfSpotLights++] = light;
}

void RenderScene::RemoveObject(uint _instance)
{
	assert(_instance < m_instances);
	mat4 invisibleTransform = mat4::Identity() * mat4::Translate(float3(0, -10, 0));
	for (int i = 0; i < 16; ++i)
	{
		m_instanceList[_instance].transform[i] = invisibleTransform[i];
	}
}

void RenderScene::BuildTLAS()
{
	m_TLAS.Build(m_instanceList, m_instances, m_bvhList, m_instances);
}

void RenderScene::FindNearest(tinybvh::Ray& _ray) const
{
	m_TLAS.Intersect(_ray);
}

bool RenderScene::IsOccluded(const tinybvh::Ray& _ray) const
{
	return m_TLAS.IsOccluded(_ray);
}

const Material& RenderScene::GetMaterial(const uint _instance, const uint _primitiveIndex, float2 /*triangleUV*/) const
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _primitiveIndex);
			const Material& mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			return mat;
		}
	case PrimitiveType::Sphere:
		{
			return m_materialManager.GetMaterial(m_sphereData[_instance]->m_spheres[_primitiveIndex].material);
			break;
		}
	case PrimitiveType::Cube:
		{
			return m_materialManager.GetMaterial(m_cubeData[_instance]->m_cubes[_primitiveIndex].material);
			break;
		}
	}
	printf("WARNING OBJECT NOT FOUND");
	return m_materialManager.GetMaterial((0)); // return the first material may fail
}

float3 RenderScene::SampleTextureOnTri(float2 _uv, const TriangleData& _hitTriangle, const FloatTexture& _texture,
                                       float _mip) const
{
	float2 uv0 = _hitTriangle.vert0->texCoords;
	float2 uv1 = _hitTriangle.vert1->texCoords;
	float2 uv2 = _hitTriangle.vert2->texCoords;

	float u = _uv.x;
	float v = _uv.y;
	float w = 1.0f - u - v;

	float interpolatedU = fmod(abs(uv0.x * w + uv1.x * u + uv2.x * v), 1.f);
	float interpolatedV = fmod(abs(uv0.y * w + uv1.y * u + uv2.y * v), 1.f);

	return float3(_texture.Sample(float2(interpolatedU, interpolatedV), _mip));
}

float4 RenderScene::SampleTextureOnTriXYZW(float2 _uv, const TriangleData& _hitTriangle, const FloatTexture& _texture,
                                           float _mip) const
{
	float2 uv0 = _hitTriangle.vert0->texCoords;
	float2 uv1 = _hitTriangle.vert1->texCoords;
	float2 uv2 = _hitTriangle.vert2->texCoords;

	float u = _uv.x;
	float v = _uv.y;
	float w = 1.0f - u - v;

	float interpolatedU = fmod(abs(uv0.x * w + uv1.x * u + uv2.x * v), 1.f);
	float interpolatedV = fmod(abs(uv0.y * w + uv1.y * u + uv2.y * v), 1.f);

	return _texture.Sample(float2(interpolatedU, interpolatedV), _mip);
}


float3 RenderScene::GetAlbedo(uint _instance, uint _index, float2 _uv, bool _mipMap, const float3& _normal,
                              const float3& _rayDir, float _rayLength, float _vFov, float _lodBias) const
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _index);
			const Material mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			if (mat.hasTextures)
			{
				const FloatTexture& texture = m_textureManager.GetTexture(mat.albedoTexture);

				float mipLevel;
				if (_mipMap)
				{
					float texelArea2 = abs(
						(hitTriangle.vert1->texCoords.x - hitTriangle.vert0->texCoords.x) * (hitTriangle.vert2->
							texCoords.y
							- hitTriangle.vert0->texCoords.y) - (hitTriangle.vert2->texCoords.x - hitTriangle.vert0->
							texCoords.x) * (hitTriangle.vert1->texCoords.y - hitTriangle.vert0->texCoords.y));
					texelArea2 = texelArea2 * texture.m_resolution.x * texture.m_resolution.y;

					float triangleArea2 = length(cross((hitTriangle.vert1->position - hitTriangle.vert0->position),
					                                   (hitTriangle.vert2->position - hitTriangle.vert0->position)));

					float texelConstant = 0.5f * log2f(texelArea2 / triangleArea2);
					float coneAngle = atan((2.f * tan(DegToRad(_vFov))) / RENDER_HEIGHT);
					mipLevel = texelConstant + log2(
						coneAngle * _rayLength * (1 / abs(dot(_normal, normalize(_rayDir)))));
					mipLevel += _lodBias;
				}
				else mipLevel = 0.f;

				return SampleTextureOnTri(_uv, hitTriangle, texture, mipLevel);
			}
			return mat.albedo;
			break;
		}
	case PrimitiveType::Sphere:
		{
			return m_materialManager.GetMaterial(m_sphereData[_instance]->m_spheres[_index].material).albedo;
		}
	case PrimitiveType::Cube:
		{
			return m_materialManager.GetMaterial(m_cubeData[_instance]->m_cubes[_index].material).albedo;
			break;
		}
	}
	printf("WARNING OBJECT NOT FOUND");
	return float3(0);
}

float3 RenderScene::GetEmission(uint _instance, uint _primitiveIndex, float2 _triangleUv) const
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _primitiveIndex);
			const Material mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			if (mat.hasTextures && mat.emissionTexture != -1 && mat.emissionStrength > 0.f)
			{
				const FloatTexture& texture = m_textureManager.GetTexture(mat.emissionTexture);
				return SampleTextureOnTri(_triangleUv, hitTriangle, texture, 0.f) * mat.emissionStrength;
			}
			else if (mat.emissionStrength > 0.f)
			{
				return mat.emissionStrength * mat.albedo;
			}
			return float3(0.f);
		}
	case PrimitiveType::Sphere:
		{
			const Material mat = m_materialManager.
				GetMaterial(m_sphereData[_instance]->m_spheres[_primitiveIndex].material);
			if (mat.emissionStrength > 0.f)
			{
				return mat.emissionStrength * mat.albedo;
			}
			return float3(0.f);
		}
	}
	return float3(0.f);
}

float RenderScene::GetReflectivity(uint _instance, uint _index, float2 _uv, bool _mipMap,
                                   const float3& _normal, const float3& _rayDir, float _rayLength, float _vFov,
                                   float _lodBias) const
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _index);
			const Material mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			if (mat.hasTextures && mat.metallicRoughnessTexture != -1)
			{
				const FloatTexture& texture = m_textureManager.GetTexture(mat.metallicRoughnessTexture);

				float mipLevel;
				if (_mipMap)
				{
					float texelArea2 = abs(
						(hitTriangle.vert1->texCoords.x - hitTriangle.vert0->texCoords.x) * (hitTriangle.vert2->
							texCoords.y
							- hitTriangle.vert0->texCoords.y) - (hitTriangle.vert2->texCoords.x - hitTriangle.vert0->
							texCoords.x) * (hitTriangle.vert1->texCoords.y - hitTriangle.vert0->texCoords.y));
					texelArea2 = texelArea2 * texture.m_resolution.x * texture.m_resolution.y;

					float triangleArea2 = length(cross((hitTriangle.vert1->position - hitTriangle.vert0->position),
					                                   (hitTriangle.vert2->position - hitTriangle.vert0->position)));

					float texelConstant = 0.5f * log2f(texelArea2 / triangleArea2);
					float coneAngle = atan((2.f * tan(DegToRad(_vFov))) / RENDER_HEIGHT);
					mipLevel = texelConstant + log2(
						coneAngle * _rayLength * (1 / abs(dot(_normal, normalize(_rayDir)))));
					mipLevel += _lodBias;
				}
				else mipLevel = 0.f;

				return SampleTextureOnTri(_uv, hitTriangle, texture, 0.f).z;
			}
			return mat.metallic;
			break;
		}
	case PrimitiveType::Sphere:
		{
			return m_materialManager.GetMaterial(m_sphereData[_instance]->m_spheres[_index].material).metallic;
		}
	case PrimitiveType::Cube:
		{
			return m_materialManager.GetMaterial(m_cubeData[_instance]->m_cubes[_index].material).metallic;
			break;
		}
	}
	printf("WARNING OBJECT NOT FOUND\n");
	return 0.f;
}

float RenderScene::GetRoughness(uint _instance, uint _index, float2 _uv, bool _mipMap, const float3& _normal,
                                const float3& _rayDir, float _rayLength, float _vFov, float _lodBias) const
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _index);
			const Material mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			if (mat.hasTextures && mat.metallicRoughnessTexture != -1)
			{
				const FloatTexture& texture = m_textureManager.GetTexture(mat.metallicRoughnessTexture);

				float mipLevel;
				if (_mipMap)
				{
					float texelArea2 = abs(
						(hitTriangle.vert1->texCoords.x - hitTriangle.vert0->texCoords.x) * (hitTriangle.vert2->
							texCoords.y
							- hitTriangle.vert0->texCoords.y) - (hitTriangle.vert2->texCoords.x - hitTriangle.vert0->
							texCoords.x) * (hitTriangle.vert1->texCoords.y - hitTriangle.vert0->texCoords.y));
					texelArea2 = texelArea2 * texture.m_resolution.x * texture.m_resolution.y;

					float triangleArea2 = length(cross((hitTriangle.vert1->position - hitTriangle.vert0->position),
					                                   (hitTriangle.vert2->position - hitTriangle.vert0->position)));

					float texelConstant = 0.5f * log2f(texelArea2 / triangleArea2);
					float coneAngle = atan((2.f * tan(DegToRad(_vFov))) / RENDER_HEIGHT);
					mipLevel = texelConstant + log2(
						coneAngle * _rayLength * (1 / abs(dot(_normal, normalize(_rayDir)))));
					mipLevel += _lodBias;
				}
				else mipLevel = 0.f;

				return SampleTextureOnTri(_uv, hitTriangle, texture, mipLevel).y;
			}
			return mat.roughness;
			break;
		}
	case PrimitiveType::Sphere:
		{
			return m_materialManager.GetMaterial(m_sphereData[_instance]->m_spheres[_index].material).roughness;
		}
	case PrimitiveType::Cube:
		{
			return m_materialManager.GetMaterial(m_cubeData[_instance]->m_cubes[_index].material).roughness;
			break;
		}
	}
	printf("WARNING OBJECT NOT FOUND\n");
	return 0.f;
}


float3 RenderScene::GetNormal(const uint _instance, const uint _primitiveIndex, const float2 _triangleUv, float3 _d,
                              float3 _I,
                              bool& _isFrontFace, bool _mipMap, float _lodBias, float _vFOV, float _rayLength)
{
	PrimitiveType type = m_typeList[_instance];
	switch (type)
	{
	case PrimitiveType::Triangles:
		{
			const TriangleData hitTriangle = GetTriangleData(_instance, _primitiveIndex);

			float3 N0 = hitTriangle.vert0->normal;
			float3 N1 = hitTriangle.vert1->normal;
			float3 N2 = hitTriangle.vert2->normal;

			float u = _triangleUv.x;
			float v = _triangleUv.y;
			float w = 1.0f - u - v;

			float3 normal = N0 * w + N1 * u + N2 * v;

			_d = *reinterpret_cast<mat4*>(m_instanceList[_instance].invTransform) * _d;

			if (dot(_d, normal) > 0.f)
			{
				normal = -normal;
				_isFrontFace = false;
			}
			else
			{
				_isFrontFace = true;
			}

			Material mat = m_materialManager.GetMaterial(hitTriangle.vert0->material);
			if (mat.hasTextures && mat.normalTexture != -1)
			{
				//Material has normal map so use it
				//create TBN matrix
				float3 T = hitTriangle.vert0->tangent;
				float3 B = hitTriangle.vert0->bitangent;
				mat4 TBN = mat4::Identity();
				TBN(0, 0) = T.x;
				TBN(0, 1) = B.x;
				TBN(0, 2) = normal.x;
				TBN(1, 0) = T.y;
				TBN(1, 1) = B.y;
				TBN(1, 2) = normal.y;
				TBN(2, 0) = T.z;
				TBN(2, 1) = B.z;
				TBN(2, 2) = normal.z;

				const FloatTexture& texture = m_textureManager.GetTexture(mat.normalTexture);
				float mipLevel;
				if (_mipMap)
				{
					float texelArea2 = abs(
						(hitTriangle.vert1->texCoords.x - hitTriangle.vert0->texCoords.x) * (hitTriangle.vert2->
							texCoords.y
							- hitTriangle.vert0->texCoords.y) - (hitTriangle.vert2->texCoords.x - hitTriangle.vert0->
							texCoords.x) * (hitTriangle.vert1->texCoords.y - hitTriangle.vert0->texCoords.y));
					texelArea2 = texelArea2 * texture.m_resolution.x * texture.m_resolution.y;

					float triangleArea2 = length(cross((hitTriangle.vert1->position - hitTriangle.vert0->position),
					                                   (hitTriangle.vert2->position - hitTriangle.vert0->position)));

					float texelConstant = 0.5f * log2f(texelArea2 / triangleArea2);
					float coneAngle = atan((2.f * tan(DegToRad(_vFOV))) / RENDER_HEIGHT);
					mipLevel = texelConstant + log2(
						coneAngle * _rayLength * (1 / abs(dot(normal, normalize(_d)))));
					mipLevel += _lodBias;
				}
				else mipLevel = 0.f;

				float3 normalMapValue = SampleTextureOnTri(_triangleUv, hitTriangle, texture, mipLevel);
				normalMapValue = normalMapValue * 2.f - 1.f;

				normal = normalMapValue * TBN;
			}
			normal = *reinterpret_cast<mat4*>(m_instanceList[_instance].transform) * normal;


			return normalize(normal);
		}
	case PrimitiveType::Sphere:
		{
			mat4 invTransform = *reinterpret_cast<mat4*>(m_instanceList[_instance].invTransform);
			_I = float3(invTransform * float4(_I, 1.f));
			_d = float3(invTransform * _d);
			float3 normal = normalize(_I - (m_sphereData[_instance]->m_spheres[_primitiveIndex].pos));
			if (dot(_d, normal) > 0.f)
			{
				normal = -normal;
				_isFrontFace = false;
			}
			else
			{
				_isFrontFace = true;
			}
			return normal * *reinterpret_cast<mat4*>(m_instanceList[_instance].transform);
		}
	case PrimitiveType::Cube:
		{
			mat4 blassInvTransform = *reinterpret_cast<mat4*>(m_instanceList[_instance].invTransform);
			mat4 cubeInvTransform = m_cubeData[_instance]->m_cubes[_primitiveIndex].invTransform;

			mat4 blassTransform = *reinterpret_cast<mat4*>(m_instanceList[_instance].transform);
			mat4 cubeTransform = m_cubeData[_instance]->m_cubes[_primitiveIndex].transform;

			_I = float4(_I, 1.f) * blassInvTransform * cubeInvTransform;

			float3 absI = fabs(_I);
			auto normal = float3(0);
			if (absI.x > absI.y && absI.x > absI.z)
			{
				normal.x = _I.x > 0.f ? 1.f : -1.f;
			}
			else if (absI.y > absI.z)
			{
				normal.y = _I.y > 0.f ? 1.f : -1.f;
			}
			else
			{
				normal.z = _I.z > 0.f ? 1.f : -1.f;
			}

			normal = normal * cubeTransform * blassTransform;

			//	-_I / max(max(abs(_I.x), abs(_I.y)), abs(_I.z));
			//normal = clamp(normal, float3(0), float3(1));
			//normal = normalize(floor(normal * (1.f + EPSILON)));


			return normal;
		}
	}
	printf("WARNING OBJECT NOT FOUND");
	return float3(0);
}

const Light& RenderScene::GetLight(uint _seed) const
{
	const uint light = RandomUInt(_seed) % (m_numberOfLights);
	return m_lights[light];
}

const LightSOA& RenderScene::GetLightSOA() const
{
	return m_lightSOA;
}


const DirLight& RenderScene::GetDirectionalLight(uint _seed) const
{
	const uint light = RandomUInt(_seed) % (m_numberOfDirectionalLights);
	return m_dirLights[light];
}

const SpotLight& RenderScene::GetSpotLight(uint _seed) const
{
	const uint light = RandomUInt(_seed) % (m_numberOfSpotLights);
	return m_spotLights[light];
}

const float4& RenderScene::GetVolumeLight(uint _seed) const
{
	assert(m_volumeBVH != nullptr);
	const uint light = RandomUInt(_seed) % (m_volumeBVH->m_lightVolumes.size());
	return m_volumeBVH->m_lightVolumes[light];
}

const VertexData& RenderScene::GetVertexData(const uint _instance, const uint _triangleIndex) const
{
	return m_vertexData[_instance][m_indices[_instance][_triangleIndex * 3]];
}

const TriangleData RenderScene::GetTriangleData(const uint _instance, const uint _triangleIndex) const
{
	TriangleData triangle;

	triangle.vert0 = &m_vertexData[_instance][m_indices[_instance][_triangleIndex * 3 + 0]];
	triangle.vert1 = &m_vertexData[_instance][m_indices[_instance][_triangleIndex * 3 + 1]];
	triangle.vert2 = &m_vertexData[_instance][m_indices[_instance][_triangleIndex * 3 + 2]];

	return triangle;
}
