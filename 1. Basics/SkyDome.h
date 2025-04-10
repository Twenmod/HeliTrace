#pragma once
#include "floattexture.h"

class SkyDome
{
public:
	SkyDome(FloatTexture* _texture);
	~SkyDome();
	float4 sample(uint& _seed, float3& _outDirection);
	FloatTexture* m_texture;
private:
	uint GetImportancePixel(uint& _seed);
	float3 PixelToDirection(uint _pixel);
	float* m_invpdf;
	float* m_cdf;
};

