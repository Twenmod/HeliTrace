#include "precomp.h"

#include "Mesh.hpp"

Mesh::Mesh(const std::vector<Vert>& _vertices, const std::vector<unsigned int>& _indices,
           const std::vector<VertexData>& _data)
{
	this->m_vertices = _vertices;
	this->m_indices = _indices;
	this->m_verticesData = _data;
}
