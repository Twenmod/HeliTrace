#pragma once

class TextureManager
{
public:
	TextureManager();
	~TextureManager();
	const FloatTexture& GetTexture(std::string _path, bool _gammaCorrect = false);
	const FloatTexture& GetTexture(int _id) const;
	int LoadTexture(std::string _file, bool _gammaCorrect);

private:
	std::vector<std::pair<FloatTexture*, std::string>> m_textures;
};
