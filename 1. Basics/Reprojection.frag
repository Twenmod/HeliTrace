#version 330
uniform sampler2D c;
uniform sampler2D N; // A is the depth buffer
uniform sampler2D P; 
// A is reprojection flag
///  0 is reproject normally
///  1 is never reproject or accumulate
///  2 is accumulate but do not reproject
//   4 is always accumulate dont check
uniform sampler2D oldC;
uniform sampler2D oldN; // A is the depth buffer
uniform sampler2D oldP;

uniform int resolutionX;
uniform int resolutionY;
uniform float maximumMipMap;


vec2 offset = 1.0 / vec2(resolutionX,resolutionY);

in vec2 u;
out vec4 f;


//Settings
uniform float pixelChangeTreshold;
uniform bool reproject;
  uniform float neighBourSimilarityTreshold;
uniform bool forceAccumulate;
uniform bool forceUpdate;


vec3 GetNewPixelColor();

float positionWeight = 0.5;
float normalWeight = 1.0;

uniform float lerp;
uniform float lerpDiff;

struct SearchResult {
	vec2 closestUV;
	vec2 secondClosestUV;
	float closestDiff;
	float secondClosestDiff;
};

struct NeighborPixel {
	vec4 color;
	float difference;
};

SearchResult processMip(vec2 uv, float mip, vec4 currNormalDepth, vec3 currPos) {

	vec2 closestFoundUV = uv;
	vec2 secondClosestFoundUV = uv;
	float closestFoundDiff = 0.0;
	float secondFoundDiff = 0.0;

	while (mip >= 0) {
		float closestDiff = 10000.0;
		vec2 closestUV = closestFoundUV;
		vec2 secondUV;
		float secondDiff = 10000.0;


		vec2 mipOffset = offset;
		if (mip > 0.0) mipOffset *= exp2(mip);
	    for (int y = -3; y <= 3; y++) {
			float nV = closestFoundUV.y + y*mipOffset.y;
			if (nV < 0.0 || nV > 1.0) continue;
			for (int x = -3; x <= 3; x++) {
				float nU = closestFoundUV.x + x*mipOffset.x;
				if (nU < 0.0 || nU > 1.0) continue;
				vec2 neighborUV = vec2(nU,nV);
				vec4 neighbourPos = textureLod(oldP, neighborUV, mip);
				if (neighbourPos.w == 1.0) continue; // do not reproject flag
				vec4 neighborNormalDepth = textureLod(oldN, neighborUV, mip).xyzw;
				float normalDiff = length(neighborNormalDepth.xyz - currNormalDepth.xyz) * normalWeight;
				float possDiff = length(neighbourPos.xyz - currPos) * positionWeight;
				float depthDiff = abs(currNormalDepth.w - neighborNormalDepth.w);
				float diff = normalDiff + possDiff;
				if (diff < closestDiff) {
					secondUV = closestUV;
					secondDiff = closestDiff;
					closestDiff = diff;
					closestUV = neighborUV;
				}
			}
        }

		closestFoundDiff = closestDiff;
		closestFoundUV = closestUV;
		secondClosestFoundUV = secondUV;
		secondFoundDiff = secondDiff;
		mip -= 1.0;
	}

	SearchResult result;
	result.closestUV = closestFoundUV;
	result.closestDiff = closestFoundDiff;
	result.secondClosestUV = secondClosestFoundUV;
	result.secondClosestDiff = secondFoundDiff;

	return result;
}


NeighborPixel FindNeighbour(vec4 currNormalDepth, vec3 currPos) {

	NeighborPixel result;
    float currentDepth = currNormalDepth.w;

    //Do a neighbourhoud search to find where this pixel came from
	SearchResult found = processMip(u,maximumMipMap, currNormalDepth, currPos);



	if (found.closestDiff > neighBourSimilarityTreshold) { // Couldnt find a similar enough neighbour so just render new pixel
		result.color =vec4(GetNewPixelColor(),1.0);
		result.difference = 0.0;
		return result;
	}
	//We dont do any processing(light, denoising etc) on oldC since it already is processed
	vec4 closestColor = texture(oldC, found.closestUV).xyzw;
	vec4 secondColor = texture(oldC, found.secondClosestUV).xyzw;
	vec3 color = mix(closestColor.xyz, secondColor.xyz, clamp(found.closestDiff/found.secondClosestDiff,0.0,1.0));
	result.color = vec4(color,closestColor.w);
	result.difference = found.closestDiff;
	return result;
	//return vec4((found.closestUV-u)*20.0,0.0,1.0);
	//return vec4(vec3(found.closestDiff),1.0);
}

vec3 GetNewPixelColor() {
	vec3 finalColor = texture (c,u).xyz;
	return finalColor;
}


void main() 
{
	//Check if the pixel changed


	vec4 normalDepth = texture(N, u);
	vec4 worldPos = texture(P, u);
	if (worldPos.w == 1.0 && !forceAccumulate) { // Do not reproject flag
		f = vec4(texture(c,u).xyz,1.0);
		return;
	}

	vec4 oldNormalDepth = texture(oldN, u);
	vec3 oldPos = texture(oldP,u).xyz;

	float normalDiff = (1.0-dot(normalDepth.xyz, oldNormalDepth.xyz)) * normalWeight;
	float possDiff = length(worldPos.xyz - oldPos) * positionWeight;
		
	float diff = normalDiff + possDiff;

	if (forceUpdate) {
		f = vec4(GetNewPixelColor(),1.0); // New pixel
		return;
	}

	vec4 color; // Alpha channel is accumulator Count
	if (diff > pixelChangeTreshold && !forceAccumulate) {
		if (reproject && worldPos.w != 2.0 && worldPos.w != 3.0) {
			NeighborPixel found = FindNeighbour(normalDepth, worldPos.xyz);
			vec4 oldPixel = found.color;
			float mixValue = lerp;
			if (oldPixel.w < 3) mixValue *= 0.5f;
			color = vec4(mix(oldPixel.xyz,GetNewPixelColor(),clamp(mixValue*max(found.difference*lerpDiff,1.0),0.0,1.0)),oldPixel.w+1);
		} else {
			color = vec4(GetNewPixelColor(),1.0); // New pixel
		}
	}else {
		vec4 oldPixel = texture(oldC,u);
		float mixValue = lerp;
		if (oldPixel.w < 3) mixValue *= 0.5f;
		color = vec4(mix(oldPixel.xyz,GetNewPixelColor(),mixValue),oldPixel.w+1);
	}

	f = color;
}