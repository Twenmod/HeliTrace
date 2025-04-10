#include "precomp.h"
#include <iostream>
#include <vector>
#include "TextureManager.h"
#include "MaterialManager.h"

#include "Model.hpp"

#include "RenderScene.h"

Model::~Model()
{
	for (int i = 0; i < m_meshes.size(); i++)
	{
		delete m_meshes[i];
	}
}

void Model::Init(const char* _path, TextureManager& _textureManager, MaterialManager& _materialManager,
                 RenderScene& _scene)
{
	LoadModel(_path, _textureManager, _materialManager, _scene);
}

void Model::Init(Mesh& _mesh)
{
	m_meshes.push_back(&_mesh);
}

std::vector<Vert> Model::GetAllVertices() const
{
	std::vector<Vert> vertices;
	for (Mesh* mesh : m_meshes)
	{
		for (Vert& vertex : mesh->m_vertices)
		{
			vertices.push_back(vertex);
		}
	}
	return vertices;
}

std::vector<uint> Model::GetAllIndices() const
{
	std::vector<uint> indices;
	for (Mesh* mesh : m_meshes)
	{
		for (uint ind : mesh->m_indices)
		{
			indices.push_back(ind);
		}
	}
	return indices;
}


void Model::LoadModel(const std::string& _path, TextureManager& _textureManager, MaterialManager& _materialManager,
                      RenderScene& _scene)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(
		_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "\x1B[31mERROR::ASSIMP::" << import.GetErrorString() << std::endl << "\x1B[37m";
		return;
	}
	m_directory = _path.substr(0, _path.find_last_of('/'));

	ProcessNode(scene->mRootNode, scene, _textureManager, _materialManager, mat4::Identity());

	for (uint i = 0; i < scene->mNumLights; ++i)
	{
		aiLight* light = scene->mLights[i];
		switch (light->mType)
		{
		case aiLightSource_POINT:
			{
				Light L;
				L.pos = *reinterpret_cast<float3*>(&light->mPosition);
				L.color = *reinterpret_cast<float3*>(&light->mColorDiffuse) * 0.0001f;
				L.size = float3(light->mSize.x);
				_scene.AddLight(L);

				break;
			}
		case aiLightSource_SPOT:
			{
				SpotLight L;
				L.pos = *reinterpret_cast<float3*>(&light->mPosition);
				L.color = *reinterpret_cast<float3*>(&light->mColorDiffuse) * 0.001f;
				L.hasTexture = false;
				L.direction = *reinterpret_cast<float3*>(&light->mDirection);
				L.innerCutoff = cos(light->mAngleInnerCone);
				L.outerCutoff = cos(light->mAngleOuterCone);
				_scene.AddSpotLight(L);
			}
		}
	}
}

void Model::ProcessNode(const aiNode* _node, const aiScene* _scene, TextureManager& _textureManager,
                        MaterialManager& _materialManager, const mat4& _transform)
{
	mat4 transform = _transform * *reinterpret_cast<const mat4*>(&_node->mTransformation);

	for (uint i = 0; i < _scene->mNumLights; ++i)
	{
		aiLight* light = _scene->mLights[i];
		if (light->mName == _node->mName)
		{
#pragma warning (push)
#pragma warning (disable:4238) // We need to take an adress of a rvalue for reinterpreting so this is intended
			light->mPosition = *reinterpret_cast<aiVector3D*>(&(transform * float4(
				*reinterpret_cast<float3*>(&light->mPosition),
				1.f)));
			light->mDirection = *reinterpret_cast<aiVector3D*>(&(transform *
				*reinterpret_cast<float3*>(&light->mDirection)));
#pragma warning (pop)
		}
	}
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < _node->mNumMeshes; i++)
	{
		aiMesh* mesh = _scene->mMeshes[_node->mMeshes[i]];
		m_meshes.push_back(ProcessMesh(mesh, _node, _scene, _textureManager, _materialManager, transform));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < _node->mNumChildren; i++)
	{
		ProcessNode(_node->mChildren[i], _scene, _textureManager, _materialManager, transform);
	}
}


Mesh* Model::ProcessMesh(aiMesh* _mesh, const aiNode* _node, const aiScene* _scene,
                         TextureManager& _textureManager, MaterialManager& _materialManager,
                         const mat4& _transform) const
{
	std::vector<Vert> vertices;
	std::vector<VertexData> verticesData;
	std::vector<unsigned int> indices;


	int matIndex = 0;

	// process material
	if (_mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = _scene->mMaterials[_mesh->mMaterialIndex];
		std::vector<int> diffuseMaps = LoadTextures(material, aiTextureType_DIFFUSE, _textureManager);
		int textureIndex = -1;
		if (diffuseMaps.size() > 0)
			textureIndex = diffuseMaps[0]; // meshes should only have 1 diffuse texture
		else textureIndex = -1;
		int normalIndex = -1;
		std::vector<int> normalMaps = LoadTextures(material, aiTextureType_NORMALS, _textureManager);
		if (normalMaps.size() > 0)
			normalIndex = normalMaps[0]; // meshes should only have 1 normal texture
		else normalIndex = -1;
		int emissionIndex = -1;
		std::vector<int> emissionMaps = LoadTextures(material, aiTextureType_EMISSIVE, _textureManager);
		if (emissionMaps.size() > 0)
			emissionIndex = emissionMaps[0]; // meshes should only have 1 normal texture
		else emissionIndex = -1;

		int pbrIndex = -1;
		std::vector<int> pbrMaps = LoadTextures(material, aiTextureType_UNKNOWN, _textureManager);
		// Unknown is the metal,roughness map because assimp has good naming convention :D
		if (pbrMaps.size() > 0)
			pbrIndex = pbrMaps[0]; // meshes should only have 1 normal texture
		else pbrIndex = -1;


		aiColor3D albedo;
		float metallicness = 0.f;
		float roughness = 0.f;
		float transmission = 0.f;
		float IOR = 0.f;
		aiColor3D emission;
		emission.r = 0.f;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
		material->Get(AI_MATKEY_METALLIC_FACTOR, metallicness);
		material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
		material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission);
		material->Get(AI_MATKEY_REFRACTI, IOR);
		material->Get(AI_MATKEY_COLOR_EMISSIVE, emission);

		if (textureIndex != -1)
		{
			matIndex = static_cast<int>(_materialManager.AddMaterial(textureIndex, normalIndex, pbrIndex, emissionIndex,
			                                                         metallicness,
			                                                         transmission, roughness, IOR,
			                                                         emissionIndex != -1 ? 10.f : 0.f));
		}
		else
		{
			matIndex = static_cast<int>(_materialManager.AddMaterial(float3(albedo.r, albedo.g, albedo.b), metallicness,
			                                                         transmission,
			                                                         roughness, IOR));
		}
	}

	//Process Animations
	Animation meshAnim;
	bool hasAnimation = false;
	if (_scene->HasAnimations())
	{
		for (uint i = 0; i < _scene->mNumAnimations; ++i)
		{
			aiAnimation* animation = _scene->mAnimations[i];
			for (uint channelI = 0; channelI < animation->mNumChannels; ++channelI)
			{
				aiNodeAnim* channel = animation->mChannels[channelI];
				if (channel->mNodeName == _node->mName) // Current mesh/node or something
				{
					for (uint keyI = 0; keyI < channel->mNumPositionKeys; ++keyI)
					{
						float3 pos = *reinterpret_cast<float3*>(&channel->mPositionKeys[keyI].mValue);
						meshAnim.positions.push_back(pos);
						if (channel->mNumPositionKeys == channel->mNumRotationKeys)
						{
							quat rot = *reinterpret_cast<quat*>(&channel->mRotationKeys[keyI].mValue);
							meshAnim.rotations.push_back(rot);
						}
						else
						{
							meshAnim.rotations.emplace_back(quat());
						}
					}
					meshAnim.duration = static_cast<float>(animation->mDuration);
					meshAnim.ticksPerSecond = static_cast<float>(animation->mTicksPerSecond);
					hasAnimation = true;
				}
			}
		}
	}

	//Process vertices
	for (unsigned int i = 0; i < _mesh->mNumVertices; i++)
	{
		Vert vertex;
		VertexData data;

		// process positions
		float3 vector;
		vector.x = _mesh->mVertices[i].x;
		vector.y = _mesh->mVertices[i].y;
		vector.z = _mesh->mVertices[i].z;
		vertex.position = vector;
		data.position = vector;
		// Normals
		if (_mesh->mNormals != nullptr)
		{
			vector.x = _mesh->mNormals[i].x;
			vector.y = _mesh->mNormals[i].y;
			vector.z = _mesh->mNormals[i].z;
			data.normal = vector;
		}
		else
		{
			data.normal = float3(0, 0, 0);
		}
		//Texture coords
		if (_mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			float2 vec;
			vec.x = _mesh->mTextureCoords[0][i].x;
			vec.y = _mesh->mTextureCoords[0][i].y;
			data.texCoords = vec;
		}
		else
			data.texCoords = float2(0.0f, 0.0f);

		//(bi)tangents
		if (_mesh->mTangents != nullptr)
		{
			vector.x = _mesh->mTangents[i].x;
			vector.y = _mesh->mTangents[i].y;
			vector.z = _mesh->mTangents[i].z;
			data.tangent = vector;

			vector.x = _mesh->mBitangents[i].x;
			vector.y = _mesh->mBitangents[i].y;
			vector.z = _mesh->mBitangents[i].z;
			data.bitangent = vector;
		}

		data.material = matIndex;

		//Push to list
		vertices.push_back(vertex);
		verticesData.push_back(data);
	}
	// process indices
	for (unsigned int i = 0; i < _mesh->mNumFaces; i++)
	{
		aiFace face = _mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	//	std::vector<Texture> specularMaps = loadMaterialTextures(material,
	//		aiTextureType_SPECULAR, TEXTURE_TYPE_SPECULAR);
	//	meshMaterial.textures.insert(meshMaterial.textures.end(), specularMaps.begin(), specularMaps.end());

	//	//Should be textureType normal not height but obj exports it incorrectly in our case we dont have heightmaps so its no issue
	//	std::vector<Texture> normalMaps = loadMaterialTextures(material,
	//		aiTextureType_HEIGHT, TEXTURE_TYPE_NORMAL);
	//	meshMaterial.textures.insert(meshMaterial.textures.end(), normalMaps.begin(), normalMaps.end());

	//	std::vector<Texture> emissionMaps = loadMaterialTextures(material,
	//		aiTextureType_EMISSIVE, TEXTURE_TYPE_EMISSIVE);
	//	meshMaterial.textures.insert(meshMaterial.textures.end(), emissionMaps.begin(), emissionMaps.end());
	//	std::vector<Texture> alphaMaps = loadMaterialTextures(material,
	//		aiTextureType_OPACITY, TEXTURE_TYPE_ALPHA);
	//	if (alphaMaps.size() > 0) 
	//		opaque = false;
	//	meshMaterial.textures.insert(meshMaterial.textures.end(), alphaMaps.begin(), alphaMaps.end());
	//}


	Mesh* createdMesh = new Mesh(vertices, indices, verticesData);
	createdMesh->m_name = _mesh->mName.C_Str();
	createdMesh->m_transformation = _transform;
	if (hasAnimation)
	{
		createdMesh->m_animation = meshAnim;
		createdMesh->m_hasAnimation = true;
	}
	return createdMesh;
}

std::vector<int> Model::LoadTextures(const aiMaterial* _mat, const aiTextureType _type,
                                     TextureManager& _textureManager) const
{
	std::vector<int> textures;
	for (unsigned int i = 0; i < _mat->GetTextureCount(_type); i++)
	{
		aiString str;
		_mat->GetTexture(_type, i, &str);
		std::string filepath = m_directory + '/' + str.C_Str();
		int texture = _textureManager.LoadTexture(filepath, true);
		textures.push_back(texture);
	}
	return textures;
}
