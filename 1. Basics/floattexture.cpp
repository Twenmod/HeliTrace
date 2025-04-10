#include "precomp.h"
#include "floattexture.h"

#include <algorithm>

#include "../lib/stb_image.h"

FloatTexture::FloatTexture(const char* _path, bool _gammaCorrect)
{
	// check if file exists; show an error if there is a problem
	FILE* f = fopen(_path, "rb");
	if (f == nullptr)
	{
		FatalError("File not found: %s", _path);
		abort();
	}
	fclose(f);
	// load the file
	// use stb_image to load the image file
	int n;
	float* data = stbi_loadf(_path, &m_resolution.x, &m_resolution.y, &n, 0);
	if (!data) return; // load failed
	m_pixels = (float4*)MALLOC64(m_resolution.x * m_resolution.y * sizeof(float4));
	if (m_pixels == nullptr)
	{
		DebugBreak();
		abort();
	}
	const int s = m_resolution.x * m_resolution.y;
	if (n == 1) /* greyscale */
		for (int i = 0; i < s; i++)
		{
			const unsigned char p = (const unsigned char)data[i];
			m_pixels[i] = float4(float3(p), 1.f);
		}
	else
	{
		if (_gammaCorrect)
		{
			float gamma = 1.0f / 2.2f;
			if (n == 3)
				for (int i = 0; i < s; i++)
					m_pixels[i] =
						float4(pow(data[i * n + 0], gamma), pow(data[i * n + 1], gamma), pow(data[i * n + 2], gamma),
						       1.f);
			else
				for (int i = 0; i < s; i++)
					m_pixels[i] = float4(pow(data[i * n + 0], gamma), pow(data[i * n + 1], gamma),
					                     pow(data[i * n + 2], gamma),
					                     data[i * n + 3]);
		}
		else
		{
			if (n == 3)
				for (int i = 0; i < s; i++)
					m_pixels[i] =
						float4(data[i * n + 0], data[i * n + 1], data[i * n + 2], 1.f);
			else
				for (int i = 0; i < s; i++)
					m_pixels[i] = float4(data[i * n + 0], data[i * n + 1], data[i * n + 2],
					                     data[i * n + 3]);
		}
	}
	// free stb_image data
	stbi_image_free(data);
}

FloatTexture::FloatTexture(int _x, int _y)
{
	m_resolution.x = _x;
	m_resolution.y = _y;
	m_pixels = static_cast<float4*>(MALLOC64(m_resolution.x * m_resolution.y * sizeof(float4)));
}

void FloatTexture::Line(float _x1, float _y1, float _x2, float _y2, const float4 _color)
{
	// clip (Cohen-Sutherland, https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm)
	const float xmin = 0, ymin = 0, xmax = (float)m_resolution.x - 1, ymax = (float)m_resolution.y - 1;
	int c0 = OUTCODE(_x1, _y1), c1 = OUTCODE(_x2, _y2);
	bool accept = false;
	while (1)
	{
		if (!(c0 | c1))
		{
			accept = true;
			break;
		}
		else if (c0 & c1) break;
		else
		{
			float x = 0, y = 0;
			const int co = c0 ? c0 : c1;
			if (co & 8) x = _x1 + (_x2 - _x1) * (ymax - _y1) / (_y2 - _y1), y = ymax;
			else if (co & 4) x = _x1 + (_x2 - _x1) * (ymin - _y1) / (_y2 - _y1), y = ymin;
			else if (co & 2) y = _y1 + (_y2 - _y1) * (xmax - _x1) / (_x2 - _x1), x = xmax;
			else if (co & 1) y = _y1 + (_y2 - _y1) * (xmin - _x1) / (_x2 - _x1), x = xmin;
			if (co == c0) _x1 = x, _y1 = y, c0 = OUTCODE(_x1, _y1);
			else _x2 = x, _y2 = y, c1 = OUTCODE(_x2, _y2);
		}
	}
	if (!accept) return;
	float b = _x2 - _x1, h = _y2 - _y1, l = fabsf(b);
	l = std::max(fabsf(h), l);
	int il = (int)l;
	float dx = b / (float)l, dy = h / (float)l;
	for (int i = 0; i <= il; i++, _x1 += dx, _y1 += dy)
	{
		*(m_pixels + static_cast<int>(_x1) + static_cast<int>(_y1) * m_resolution.x) = _color;
	}
}

FloatTexture::~FloatTexture()
{
	FREE64(m_pixels);
	delete m_mips;
}

void FloatTexture::Clear(const float4 _c)
{
	const int s = m_resolution.x * m_resolution.y;
	for (int i = 0; i < s; i++) m_pixels[i] = _c;
}

float4 FloatTexture::SampleSphere(const float3 _D) const
{
	const float u = 0.5f + atan2(_D.x, _D.z) / (2.0f * PI);
	const float v = 0.5f - asin(_D.y) / PI;

	//TODO: interp
	const int x = min(static_cast<int>(round(u * m_resolution.x)), m_resolution.x - 1);
	const int y = min(static_cast<int>(round(v * m_resolution.y)), m_resolution.y - 1);

	return m_pixels[x + y * m_resolution.x];
}

float4 FloatTexture::Sample(float2 _uv, float _mip) const
{
	_mip = clamp(_mip, 0.f, static_cast<float>(m_mipLevels) - 1.f);
	int2 textureSize = m_resolution;
	if (_mip > 1.f)
	{
		float reciprical = 1.f / exp2f(floor(_mip));
		textureSize.x = static_cast<int>(floor(static_cast<float>(textureSize.x) * reciprical));
		textureSize.y = static_cast<int>(floor(static_cast<float>(textureSize.y) * reciprical));
	}

	_uv.x = fmod(_uv.x, 1.0f);
	_uv.y = fmod(_uv.y, 1.0f);

	int x = static_cast<int>(_uv.x * (textureSize.x));
	int y = static_cast<int>(_uv.y * (textureSize.y));

	//Get the remainder of the coordinates (so the coords in between the pixels)
	float xPart = _uv.x * textureSize.x - static_cast<int>(_uv.x * (textureSize.x));
	float yPart = _uv.y * textureSize.y - static_cast<int>(_uv.y * (textureSize.y));

	float4 p00, p10, p01, p11;
	if (floor(_mip) == 0.f)
	{
		int x0 = x;
		int y0 = y;
		int x1 = x + 1 < textureSize.x ? x + 1 : 0;
		int y1 = y + 1 < textureSize.y ? y + 1 : 0;

		p00 = m_pixels[x0 + y0 * textureSize.x];
		p10 = m_pixels[x1 + y0 * textureSize.x];
		p01 = m_pixels[x0 + y1 * textureSize.x];
		p11 = m_pixels[x1 + y1 * textureSize.x];
	}
	else
	{
		int i00;
		if (floor(_mip) < 2.f)
			i00 = 0;
		else
			i00 = m_mipOffsets[static_cast<int>(floor(_mip)) - 2];

		int x0 = x;
		int y0 = y;
		int x1 = x + 1 < textureSize.x ? x + 1 : 0;
		int y1 = y + 1 < textureSize.y ? y + 1 : 0;

		p00 = m_mips[i00 + x0 + y0 * textureSize.x];
		p10 = m_mips[i00 + x1 + y0 * textureSize.x];
		p01 = m_mips[i00 + x0 + y1 * textureSize.x];
		p11 = m_mips[i00 + x1 + y1 * textureSize.x];
	}
	float4 top = lerp(p00, p10, xPart);
	float4 bottom = lerp(p01, p11, xPart);
	float4 color = lerp(top, bottom, yPart);

	if (_mip > 0.f && (floor(_mip) + 1) < static_cast<float>(m_mipLevels - 1))
	{
		//interpolate mips
		float reciprical = 1.f / exp2f(floor(_mip + 1.f));
		textureSize = m_resolution;
		textureSize.x = static_cast<int>(floor(static_cast<float>(textureSize.x) * reciprical));
		textureSize.y = static_cast<int>(floor(static_cast<float>(textureSize.y) * reciprical));
		int x2 = static_cast<int>(_uv.x * static_cast<float>((textureSize.x)));
		int y2 = static_cast<int>(_uv.y * static_cast<float>((textureSize.y)));

		//Get the remainder of the coordinates (so the coords in between the pixels)
		float xPart2 = _uv.x * textureSize.x - static_cast<int>(_uv.x * (textureSize.x));
		float yPart2 = _uv.y * textureSize.y - static_cast<int>(_uv.y * (textureSize.y));

		int i00;
		if (floor(_mip + 1.f) < 2.f)
			i00 = 0;
		else
			i00 = m_mipOffsets[static_cast<int>(floor(_mip + 1.f)) - 2];

		int x0 = x2;
		int y0 = y2;
		int x1 = x2 + 1 < textureSize.x ? x2 + 1 : 0;
		int y1 = y2 + 1 < textureSize.y ? y2 + 1 : 0;

		p00 = m_mips[i00 + x0 + y0 * textureSize.x];
		p10 = m_mips[i00 + x1 + y0 * textureSize.x];
		p01 = m_mips[i00 + x0 + y1 * textureSize.x];
		p11 = m_mips[i00 + x1 + y1 * textureSize.x];

		float4 top2 = lerp(p00, p10, xPart2);
		float4 bottom2 = lerp(p01, p11, xPart2);
		float4 color2 = lerp(top2, bottom2, yPart2);
		color = lerp(color, color2, _mip - floor(_mip));
	}

	return color;
}

void FloatTexture::SetChar(int _c, const char* _c1, const char* _c2, const char* _c3, const char* _c4, const char* _c5)
{
	strcpy(m_font[_c][0], _c1);
	strcpy(m_font[_c][1], _c2);
	strcpy(m_font[_c][2], _c3);
	strcpy(m_font[_c][3], _c4);
	strcpy(m_font[_c][4], _c5);
}

void FloatTexture::InitCharset()
{
	SetChar(0, ":ooo:", "o:::o", "ooooo", "o:::o", "o:::o");
	SetChar(1, "oooo:", "o:::o", "oooo:", "o:::o", "oooo:");
	SetChar(2, ":oooo", "o::::", "o::::", "o::::", ":oooo");
	SetChar(3, "oooo:", "o:::o", "o:::o", "o:::o", "oooo:");
	SetChar(4, "ooooo", "o::::", "oooo:", "o::::", "ooooo");
	SetChar(5, "ooooo", "o::::", "ooo::", "o::::", "o::::");
	SetChar(6, ":oooo", "o::::", "o:ooo", "o:::o", ":ooo:");
	SetChar(7, "o:::o", "o:::o", "ooooo", "o:::o", "o:::o");
	SetChar(8, "::o::", "::o::", "::o::", "::o::", "::o::");
	SetChar(9, ":::o:", ":::o:", ":::o:", ":::o:", "ooo::");
	SetChar(10, "o::o:", "o:o::", "oo:::", "o:o::", "o::o:");
	SetChar(11, "o::::", "o::::", "o::::", "o::::", "ooooo");
	SetChar(12, "oo:o:", "o:o:o", "o:o:o", "o:::o", "o:::o");
	SetChar(13, "o:::o", "oo::o", "o:o:o", "o::oo", "o:::o");
	SetChar(14, ":ooo:", "o:::o", "o:::o", "o:::o", ":ooo:");
	SetChar(15, "oooo:", "o:::o", "oooo:", "o::::", "o::::");
	SetChar(16, ":ooo:", "o:::o", "o:::o", "o::oo", ":oooo");
	SetChar(17, "oooo:", "o:::o", "oooo:", "o:o::", "o::o:");
	SetChar(18, ":oooo", "o::::", ":ooo:", "::::o", "oooo:");
	SetChar(19, "ooooo", "::o::", "::o::", "::o::", "::o::");
	SetChar(20, "o:::o", "o:::o", "o:::o", "o:::o", ":oooo");
	SetChar(21, "o:::o", "o:::o", ":o:o:", ":o:o:", "::o::");
	SetChar(22, "o:::o", "o:::o", "o:o:o", "o:o:o", ":o:o:");
	SetChar(23, "o:::o", ":o:o:", "::o::", ":o:o:", "o:::o");
	SetChar(24, "o:::o", "o:::o", ":oooo", "::::o", ":ooo:");
	SetChar(25, "ooooo", ":::o:", "::o::", ":o:::", "ooooo");
	SetChar(26, ":ooo:", "o::oo", "o:o:o", "oo::o", ":ooo:");
	SetChar(27, "::o::", ":oo::", "::o::", "::o::", ":ooo:");
	SetChar(28, ":ooo:", "o:::o", "::oo:", ":o:::", "ooooo");
	SetChar(29, "oooo:", "::::o", "::oo:", "::::o", "oooo:");
	SetChar(30, "o::::", "o::o:", "ooooo", ":::o:", ":::o:");
	SetChar(31, "ooooo", "o::::", "oooo:", "::::o", "oooo:");
	SetChar(32, ":oooo", "o::::", "oooo:", "o:::o", ":ooo:");
	SetChar(33, "ooooo", "::::o", ":::o:", "::o::", "::o::");
	SetChar(34, ":ooo:", "o:::o", ":ooo:", "o:::o", ":ooo:");
	SetChar(35, ":ooo:", "o:::o", ":oooo", "::::o", ":ooo:");
	SetChar(36, "::o::", "::o::", "::o::", ":::::", "::o::");
	SetChar(37, ":ooo:", "::::o", ":::o:", ":::::", "::o::");
	SetChar(38, ":::::", ":::::", "::o::", ":::::", "::o::");
	SetChar(39, ":::::", ":::::", ":ooo:", ":::::", ":ooo:");
	SetChar(40, ":::::", ":::::", ":::::", ":::o:", "::o::");
	SetChar(41, ":::::", ":::::", ":::::", ":::::", "::o::");
	SetChar(42, ":::::", ":::::", ":ooo:", ":::::", ":::::");
	SetChar(43, ":::o:", "::o::", "::o::", "::o::", ":::o:");
	SetChar(44, "::o::", ":::o:", ":::o:", ":::o:", "::o::");
	SetChar(45, ":::::", ":::::", ":::::", ":::::", ":::::");
	SetChar(46, "ooooo", "ooooo", "ooooo", "ooooo", "ooooo");
	SetChar(47, "::o::", "::o::", ":::::", ":::::", ":::::"); // Tnx Ferry
	SetChar(48, "o:o:o", ":ooo:", "ooooo", ":ooo:", "o:o:o");
	SetChar(49, "::::o", ":::o:", "::o::", ":o:::", "o::::");
	SetChar(50, ":oo::", ":o:::", ":o:::", ":o:::", ":oo::");
	SetChar(51, "::oo:", ":::o:", ":::o:", ":::o:", "::oo:");
	char c[] = "abcdefghijklmnopqrstuvwxyz0123456789!?:=,.-() #'*/[]";
	int i;
	for (i = 0; i < 256; i++) m_transl[i] = 45;
	for (i = 0; i < 52; i++) m_transl[(unsigned char)c[i]] = i;
}


void FloatTexture::Print(const char* s, int _x1, int _y1, float4 _c, float4 _shadowColor)
{
	if (!m_fontInitialized)
	{
		// we will initialize the font on first use
		InitCharset();
		m_fontInitialized = true;
	}
	float4* t = m_pixels + _x1 + _y1 * m_resolution.x;
	for (int i = 0; i < (int)(strlen(s)); i++, t += 6)
	{
		int pos = 0;
		if ((s[i] >= 'A') && (s[i] <= 'Z')) pos = m_transl[(unsigned short)(s[i] - ('A' - 'a'))];
		else pos = m_transl[(unsigned short)s[i]];
		float4* a = t;
		const char* u = (const char*)m_font[pos];
		for (int v = 0; v < 5; v++, u++, a += m_resolution.x)
			for (int h = 0; h < 5; h++)
				if (*u++ == 'o')
					*(a + h) = _c, *(a + h + m_resolution.x) = lerp(*(a + h + m_resolution.x), _shadowColor,
					                                                _shadowColor.w);
	}
}

void FloatTexture::Bar(int _x1, int _y1, int _x2, int _y2, float4 _c)
{
	// clipping
	if (_x1 < 0) _x1 = 0;
	if (_x2 >= m_resolution.x) _x2 = m_resolution.x - 1;
	if (_y1 < 0) _y1 = 0;
	if (_y2 >= m_resolution.y) _y2 = m_resolution.x - 1;
	// draw clipped bar
	float4* a = _x1 + _y1 * m_resolution.x + m_pixels;
	for (int y = _y1; y <= _y2; y++)
	{
		for (int x = 0; x <= (_x2 - _x1); x++) a[x] = _c;
		a += m_resolution.x;
	}
}

void FloatTexture::Pixel(int _x, int _y, float4 _c)
{
	float4* a = _x + _y * m_resolution.x + m_pixels;
	*a = _c;
}

void FloatTexture::GenerateMipMaps()
{
	if (m_resolution.x % 2 != 0 || m_resolution.y % 2 != 0)
	{
		printf("WARNING: Cannot generate mipmaps for texture not divisible by 2");
		return;
	}

	m_mipLevels = static_cast<int>(floor(log2f(static_cast<float>(m_resolution.x))));
	m_mips = new float4[m_resolution.x * m_resolution.y];
	m_mipOffsets = new int[m_mipLevels - 2];
	//Generate the first mip
	int2 topSize = m_resolution;
	int2 mipSize = int2(m_resolution.x / 2, m_resolution.y / 2);
	int mipStart = 0;

	for (int y = 0; y < mipSize.y; ++y) // Mip 1
	{
		for (int x = 0; x < mipSize.x; ++x)
		{
			int i00 = x * 2 + (y * 2) * (topSize.x);

			float4 p00 = m_pixels[i00];
			float4 p10 = m_pixels[i00 + 1];
			float4 p01 = m_pixels[i00 + topSize.x];
			float4 p11 = m_pixels[i00 + topSize.x + 1];
			float4 top = lerp(p00, p10, 0.5f);
			float4 bottom = lerp(p01, p11, 0.5f);
			float4 color = lerp(top, bottom, 0.5f);
			m_mips[mipStart + x + y * mipSize.x] = color;
		}
	}

	for (int mip = 2; mip < m_mipLevels; ++mip)
	{
		topSize = mipSize;
		mipSize = int2(topSize.x / 2, topSize.y / 2);
		int topStart = mipStart;
		mipStart += topSize.x * topSize.y;
		m_mipOffsets[mip - 2] = mipStart;
		for (int y = 0; y < mipSize.y; ++y)
		{
			for (int x = 0; x < mipSize.x; ++x)
			{
				int i00 = topStart + x * 2 + (y * 2) * (topSize.x);

				float4 p00 = m_mips[i00];
				float4 p10 = m_mips[i00 + 1];
				float4 p01 = m_mips[i00 + topSize.x];
				float4 p11 = m_mips[i00 + topSize.x + 1];
				float4 top = lerp(p00, p10, 0.5f);
				float4 bottom = lerp(p01, p11, 0.5f);
				float4 color = lerp(top, bottom, 0.5f);
				m_mips[mipStart + x + y * mipSize.x] = color;
			}
		}
	}
}
