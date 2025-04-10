#version 330
uniform sampler2D c;
uniform sampler2D n;
uniform sampler2D p;

uniform int resolutionX;
uniform int resolutionY;


vec2 offset = 1.0 / vec2(resolutionX,resolutionY);

in vec2 u;
out vec4 f;

uniform float amount;
uniform float fallSpeed;
uniform float focusDistance;
uniform bool horizontal;

int size = 32;

const float weights[33] = float[]( 
0.0702, 0.0699, 0.0691, 0.0676, 0.0657, 0.0633, 0.0605, 0.0573, 
 0.0539, 0.0502, 0.0464, 0.0426, 0.0387, 0.0349, 0.0312, 0.0277, 
 0.0244, 0.0213, 0.0184, 0.0158, 0.0134, 0.0113, 0.0095, 0.0079, 
 0.0065, 0.0053, 0.0043, 0.0035, 0.0028, 0.0022, 0.0017, 0.0013, 0.0010
);

void main() {

	vec2 direction;
	if (horizontal) direction = vec2(1.0,0.0);
	else direction = vec2(0.0,1.0);

	//Positive direction
	float totalWeight = 0.0;
	vec3 totalColor = vec3(0.0);

	bool blurMax = (texture(p,u).w == 1.0);

	//Positive
	for(int i = 0; i <= size; i++) {
		float weight = weights[i];
		vec2 uv = u + direction*i*offset;
		if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0) break; // break off chain at screen edge
		weight *= 1.0/(1.0+(i/amount));
		totalWeight += weight;
		totalColor += texture(c,uv).xyz * weight;
	}

	//Negative
	for(int i = 0; i <= size; i++) {
		float weight = weights[i];
		vec2 uv = u + -direction*i*offset;
		if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0) break; // break off chain at screen edge
		weight *= 1.0/(1.0+(i/amount));
		totalWeight += weight;
		totalColor += texture(c,uv).xyz * weight;
	}

	float blurAmount = abs(texture(n,u).w - focusDistance);
	blurAmount = blurAmount * blurAmount;
	blurAmount = min(blurAmount*fallSpeed, 1.0);

	vec4 baseColor = texture(c,u);
	f = mix(vec4(baseColor.xyz,1.0), vec4(totalColor / totalWeight,1.0), blurAmount);
}