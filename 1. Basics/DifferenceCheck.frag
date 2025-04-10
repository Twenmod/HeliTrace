#version 330
uniform sampler2D c;
uniform sampler2D ctest;

in vec2 u;
out vec4 f;

void main() {
	vec4 tex = texture(c,u);
	vec4 tex2 = texture(ctest,u);
	vec3 color = vec3(length(tex.xyz-tex2.xyz));
	f = vec4(color,1.0);
}