#pragma once
class FloatTexture
{
public:
	FloatTexture(const char* _path, bool _gammaCorrect = false);
	FloatTexture(int _x, int _y);
	~FloatTexture();
	void Clear(float4 _c);
	void Line(float _x1, float _y1, float _x2, float _y2, float4 _color);
	float4 SampleSphere(float3 _direction) const;

	float4 Sample(float2 _uv, float _mip) const;
	void SetChar(int _c, const char* _c1, const char* _c2, const char* _c3, const char* _c4, const char* _c5);
	void InitCharset();
	void Print(const char* _t, int _x1, int _y1, float4 _c, float4 _shadowColor);
	void Bar(int _x1, int _y1, int _x2, int _y2, float4 _c);
	void Pixel(int _x, int _y, float4 _c);

	void GenerateMipMaps();

	int2 m_resolution{int2(0)};
	float4* m_pixels{nullptr};
	float4* m_mips{nullptr};
	int* m_mipOffsets;
	int m_mipLevels{1};

private:
	// static data for the hardcoded font
	static inline char m_font[52][5][6];
	static inline int m_transl[256];
	static inline bool m_fontInitialized = false;
};
