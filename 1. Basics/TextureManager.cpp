#include "precomp.h"
#include "TextureManager.h"
#include "surface.h"

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
	for (int i = 0; i < m_textures.size(); ++i)
	{
		delete m_textures[i].first;
	}
}

const FloatTexture& TextureManager::GetTexture(std::string _path, bool _gammaCorrect)
{
	auto it = std::find_if(m_textures.begin(), m_textures.end(),
	                       [&_path](const std::pair<FloatTexture*, std::string>& elem)
	                       {
		                       return elem.second == _path;
	                       });
	if (it == m_textures.end())
	{
		return *m_textures[LoadTexture(_path, _gammaCorrect)].first;
	}
	printf("WARNING TEXTURE NOT FOUND");
	return *m_textures[0].first;
}

const FloatTexture& TextureManager::GetTexture(int id) const
{
	return *m_textures[id].first;
}

int TextureManager::LoadTexture(std::string file, bool _gammaCorrect)
{
	int i = 0;
	for (auto texture : m_textures)
	{
		if (texture.second == file)
		{
			return i;
		}
		i++;
	}


	m_textures.push_back(
		std::make_pair<FloatTexture*, std::string&>(new FloatTexture(file.c_str(), _gammaCorrect), file));

	m_textures[m_textures.size() - 1].first->GenerateMipMaps();

	return (static_cast<int>(m_textures.size() - 1));
}
