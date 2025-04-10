#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.hpp"

//Slightly modified from LearnOpengl
//https://learnopengl.com/Model-Loading/Model

class RenderScene;
class TextureManager;
class MaterialManager;

class Model
{
public:
	Model()
	= default;

	~Model();
	void Init(const char* _path, TextureManager& _textureManager, MaterialManager& _materialManager,
	          RenderScene& _scene);
	void Init(Mesh& _mesh);

	std::vector<Vert> GetAllVertices() const;
	std::vector<uint> GetAllIndices() const;

	std::vector<Mesh*> GetMeshes() { return m_meshes; }
	std::vector<Mesh*> m_meshes;

	//void SetupInstanceData(unsigned int dataBuffer, unsigned int location, unsigned int size = 3, void* offset = (void*)0);
private:
	// model data
	std::string m_directory;

	void LoadModel(const std::string& _path, TextureManager& _textureManager, MaterialManager& _materialManager,
	               RenderScene& _scene);
	void ProcessNode(const aiNode* _node, const aiScene* _scene, TextureManager& _textureManager,
	                 MaterialManager& _materialManager, const mat4& _transform);
	Mesh* ProcessMesh(aiMesh* _mesh, const aiNode* _node, const aiScene* _scene,
	                  TextureManager& _textureManager, MaterialManager& _materialManager, const mat4& _transform) const;
	std::vector<int> LoadTextures(const aiMaterial* _mat, aiTextureType _type, TextureManager& _textureManager) const;
};
