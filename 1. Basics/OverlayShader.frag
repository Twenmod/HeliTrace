#version 330
uniform sampler2D c;
uniform sampler2D overlay;

in vec2 u;
out vec4 f;


void main() {
	vec4 tex = texture(c,u);
	vec4 overlay = texture(overlay,u);

	vec3 color;
	if (overlay.w == -1) color = 1-tex.xyz;
	else color = mix(tex.xyz, overlay.xyz, overlay.w);

	f = vec4(color,1.0);
}