#version 330
uniform sampler2D c;
uniform sampler2D n;
uniform sampler2D p;

uniform int resolutionX;
uniform int resolutionY;


vec2 offset = 1.0 / vec2(resolutionX,resolutionY);

in vec2 u;
out vec4 f;

uniform float edgeFallSpeed;
uniform float posFallSpeed;
uniform bool horizontal;
uniform bool splitColor;
uniform bool combineColor;

int sizeMax = 20;
int sizeMin = 4;
int maxSamples = 30;

const float weights[33] = float[]( 
0.0702, 0.0699, 0.0691, 0.0676, 0.0657, 0.0633, 0.0605, 0.0573, 
 0.0539, 0.0502, 0.0464, 0.0426, 0.0387, 0.0349, 0.0312, 0.0277, 
 0.0244, 0.0213, 0.0184, 0.0158, 0.0134, 0.0113, 0.0095, 0.0079, 
 0.0065, 0.0053, 0.0043, 0.0035, 0.0028, 0.0022, 0.0017, 0.0013, 0.0010
);

vec4 sampleTex(vec2 uv) {
	return texture(c,uv);
}

void main() {


	vec2 direction;
	if (horizontal) direction = vec2(1.0,0.0);
	else direction = vec2(0.0,1.0);

	//Positive direction
	float totalWeight = 0.0;
	vec3 totalColor = vec3(0.0);

	vec4 centerColor = sampleTex(u);

	int size = int(mix(sizeMax,sizeMin,clamp(centerColor.w/maxSamples,0.0,1.0)));


	vec4 centerP = texture(p,u);
	if (centerP.w == 1.0){
		f = centerColor;
		return;
	}

	//Positive
	float weightFalloff = 1.0;
	vec2 lastUV = vec2(0);
	for(int i = 0; i <= size; i++) {
		float weight = weights[i];
		vec2 uv = u + direction*i*offset;
		if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0) break; // break off chain at screen edge
		if (i != 0) {
			vec4 P = texture(p,uv);
			if (P.w == 1.0 || (P.w == 3.0 && centerP.w != 3.0) || (P.w != 3.0 && centerP.w == 3.0)) break; // break off at non denoisable spots

			float normalDiff = 0.0;
			vec4 oldNormal = texture(n,vec2(lastUV.x,lastUV.y)).xyzw;
			vec4 normal = texture(n,vec2(uv.x,uv.y)).xyzw;
			normalDiff = 1.0-dot(oldNormal.xyz, normal.xyz);
			vec4 oldPos = texture(p,vec2(lastUV.x,lastUV.y)).xyzw;
			vec4 pos = texture(p,vec2(uv.x,uv.y)).xyzw;
			float positionDiff = length(oldPos.xyz/oldNormal.w-pos.xyz/normal.w);
			weightFalloff -= normalDiff * edgeFallSpeed + positionDiff * posFallSpeed;
			if (weightFalloff <= 0.0) break; // break off chain at edges
			weight *= weightFalloff;
		}
		totalWeight += weight;
		totalColor += sampleTex(uv).xyz * weight;
		lastUV = uv;
	}

	//Negative
	weightFalloff = 1.0;
	lastUV = vec2(0);
	for(int i = 0; i <= size; i++) {
		float weight = weights[i];
		vec2 uv = u + -direction*i*offset;
		if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0) break; // break off chain at screen edge
		if (i != 0) {
			vec4 P = texture(p,uv);
			if (P.w == 1.0 || (P.w == 3.0 && centerP.w != 3.0) || (P.w != 3.0 && centerP.w == 3.0)) break; // break off at non denoisable spots

			float normalDiff = 0.0;
			vec4 oldNormal = texture(n,vec2(lastUV.x,lastUV.y)).xyzw;
			vec4 normal = texture(n,vec2(uv.x,uv.y)).xyzw;
			normalDiff = 1.0-dot(oldNormal.xyz, normal.xyz);
			vec4 oldPos = texture(p,vec2(lastUV.x,lastUV.y)).xyzw;
			vec4 pos = texture(p,vec2(uv.x,uv.y)).xyzw;
			float positionDiff = length(oldPos.xyz/oldNormal.w-pos.xyz/normal.w);
			weightFalloff -= normalDiff * edgeFallSpeed + positionDiff * posFallSpeed;
			if (weightFalloff <= 0.0) break; // break off chain at edges
			weight *= weightFalloff;
		}
		totalWeight += weight;
		totalColor += sampleTex(uv).xyz * weight;
		lastUV = uv;
	}

		f = vec4(totalColor / totalWeight,centerColor.w);
}