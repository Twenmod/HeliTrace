#version 330
uniform sampler2D c;
uniform sampler2D l;
uniform sampler2D N; // A is the depth buffer
uniform sampler2D P; // A is do not reproject flag
uniform sampler2D oldC;
uniform sampler2D oldN; // A is the depth buffer
uniform sampler2D oldP;

uniform int resolutionX;
uniform int resolutionY;
uniform int maximumMipMap;

uniform float deltaTime;


vec2 offset = 1.0 / vec2(resolutionX,resolutionY);

in vec2 u;
out vec4 f;


//Settings
uniform float accumulatorLerp;
uniform float pixelChangeTreshold;
uniform bool tonemap;
  uniform float exposure;
uniform bool reproject;
  uniform float neighBourSimilarityTreshold;

vec3 GetNewPixelColor();

int searchSize;


vec3 processMip(vec2 uv, float mip, vec4 currNormalDepth, vec3 currPos) {

	vec2 testUV = uv;
	float diff = 0.0;
	while (mip > 0) {
		float closestDiff = 10000.0;
		vec2 closestUV = testUV;

		vec2 mipOffset = offset / exp2(mip);
	    for (int y = -1; y <= 1; y++) {
			float nV = uv.y + y*mipOffset.y;
			if (nV < 0.0 || nV > 1.0) continue;
			for (int x = -1; x <= 1; x++) {
				float nU = u.x + x*mipOffset.x;
				if (nU < 0.0 || nU > 1.0) continue;
				vec2 neighborUV = vec2(nU,nV);
				vec4 neighbourPos = textureLod(oldP, neighborUV, mip);
				if (neighbourPos.w == 1.0) continue; // do not reproject flag
				vec3 neighborDepth = textureLod(oldN, neighborUV, mip).xyz;
				float depthDiff = length(neighborDepth.xyz - currNormalDepth.xyz);
				float possDiff = length(neighbourPos.xyz - currPos);
				float diff = depthDiff + possDiff;
				if (diff < closestDiff) {
					closestDiff = diff;
					closestUV = neighborUV;
				}
			}
        }
		diff = closestDiff;
		testUV = closestUV;
		mip -= 1.0;
    }

	return vec3(testUV,diff);
}


vec3 FindNeighbour(vec4 currNormalDepth, vec3 currPos) {

    float currentDepth = currNormalDepth.w;

    //Do a neighbourhoud search to find where this pixel came from
	vec3 found = processMip(u,maximumMipMap, currNormalDepth, currPos);

	if (found.z > neighBourSimilarityTreshold) { // Couldnt find a similar enough neighbour so just render new pixel
		return GetNewPixelColor();
	}
	//We dont do any processing(light, denoising etc) on oldC since it already is processed
	return texture(oldC, found.xy).xyz;
}

//Tonemapping from Tamara
vec3 AcesToneMapping(vec3 color) {
	color *= exposure;
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color  + 0.59) + 0.14);
    return clamp(color, 0.0, 1.0);
}

vec3 GetNewPixelColor() {
	vec3 finalColor = texture (c,u).xyz * texture(l,u).xyz;

	if (tonemap) {
		finalColor = AcesToneMapping(finalColor);
	}
	return finalColor;
}


void main() 
{
	//Check if the pixel changed

	vec4 normalDepth = texture(N, u);
	vec4 worldPos = texture(P, u);
	if (worldPos.w == 1.0) {
		f = vec4(AcesToneMapping(texture(c,u).xyz * texture(l,u).xyz),1.0);
		return;
	}

	vec4 oldNormalDepth = texture(oldN, u);
	vec3 oldPos = texture(oldP,u).xyz;

	float depthDiff = length(normalDepth.xyz - oldNormalDepth.xyz);
	float possDiff = length(worldPos.xyz - oldPos);
		
	float diff = depthDiff + possDiff;

	float lerpA = accumulatorLerp;

	vec3 color;
	if (diff > pixelChangeTreshold) {
		if (reproject) {
			color = mix(FindNeighbour(normalDepth, worldPos.xyz), GetNewPixelColor(), lerpA);
		} else {
			color = GetNewPixelColor();
		}
	}else {
		color = mix(texture(oldC,u).xyz, GetNewPixelColor(), lerpA);
	}

	f = vec4(color,1.f);
}