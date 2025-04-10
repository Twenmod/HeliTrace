#pragma once

struct Vert
{
	float4 position;
};

__declspec(align(64)) struct VertexData
{
	float3 position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 texCoords;
	int material;
};

struct Animation
{
	std::vector<float3> positions;
	std::vector<quat> rotations;
	float duration;
	float ticksPerSecond;
};

class Mesh
{
public:
	// mesh data
	std::vector<Vert> m_vertices;
	std::vector<VertexData> m_verticesData;
	std::vector<unsigned int> m_indices;
	Animation m_animation;
	mat4 m_transformation;
	bool m_hasAnimation = false;
	std::string m_name;

	Mesh(const std::vector<Vert>& _vertices, const std::vector<unsigned int>& _indices,
	     const std::vector<VertexData>& _data);

private:
};
