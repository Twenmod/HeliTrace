#version 330
uniform sampler2D c;

in vec2 u;
out vec4 f;

void main() {
	vec4 color = texture(c,u);
	f = color;
}