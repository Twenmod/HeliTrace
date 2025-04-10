#version 330
uniform sampler2D c;
uniform sampler2D l;

in vec2 u;
out vec4 f;

uniform bool abberation;
  uniform float distortion;

uniform bool tonemap;
  uniform float exposure;

uniform bool vignetting;
  uniform float vignetteSize;
  uniform float vignetteSmooth;

uniform bool scanLines;
uniform bool uvDistort;


//Abberation from https://www.shadertoy.com/view/wsdBWM
vec2 PincushionDistortion(in vec2 uv, float strength) 
{
	vec2 st = uv - 0.5;
    float uvA = atan(st.x, st.y);
    float uvD = dot(st, st);
    return 0.5 + vec2(sin(uvA), cos(uvA)) * sqrt(uvD) * (1.0 - strength * uvD);
}

vec3 ChromaticAbberation(sampler2D tex, sampler2D tex2, in vec2 uv) 
{
	float rChannel = texture(tex, PincushionDistortion(uv, 0.15 * distortion)).r * texture(tex2, PincushionDistortion(uv, 0.15 * distortion)).r;
    float gChannel = texture(tex, PincushionDistortion(uv, 0.0 * distortion)).g * texture(tex2, PincushionDistortion(uv, 0.0 * distortion)).g;
    float bChannel = texture(tex, PincushionDistortion(uv, -0.15 * distortion)).b * texture(tex2, PincushionDistortion(uv, -0.15 * distortion)).b;
    vec3 retColor = vec3(rChannel, gChannel, bChannel);
    return retColor;
}


//Tonemapping from Tamara
vec3 AcesToneMapping(vec3 color) {
	color *= exposure;
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color  + 0.59) + 0.14);
    return clamp(color, 0.0, 1.0);
}

vec3 Vignetting(vec3 color, vec2 uv) {
	vec2 middleUV = (0.5-abs((uv.xy - 0.5)))*2.0;
	float vignette = middleUV.x * middleUV.y;
	vignette *= vignetteSize;
	vignette = pow(vignette, vignetteSmooth);

	return clamp(vignette,0.0,1.0) * color;
}


vec2 CRTCurveUV( vec2 uv )
{
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs( uv.yx ) / vec2( 6.0, 4.0 );
    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;
    return uv;
}
void DrawScanline( inout vec3 color, vec2 uv )
{
    float scanline 	= clamp( 0.95 + 0.05 * cos( 3.14 * ( uv.y + 0.008 * 1 ) * 240.0 * 1.0 ), 0.0, 1.0 );
    float grille 	= 0.85 + 0.15 * clamp( 1.5 * cos( 3.14 * uv.x * 640.0 * 1.0 ), 0.0, 1.0 );    
    color *= scanline * grille * 1.2;
}

void main() {
	
	vec2 uv = u;

	if (uvDistort) {
		uv = CRTCurveUV(u);
	}
	
	vec4 tex = texture(c,uv) * texture(l,uv);
	vec3 color = tex.xyz;

	if (abberation) {
		color = ChromaticAbberation(c, l, uv);
	}

	if (tonemap) {
		color = AcesToneMapping(color);
	}

	if(scanLines) {
		DrawScanline(color, u);
	}

	if (vignetting) {
		color = Vignetting(color, u);
	}
	f = vec4(color,1.0);
}