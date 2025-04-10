#version 330
in vec4 p;
in vec2 t;
out vec2 u;

uniform bool Flip;


void main()
{
	if (Flip) 	u = vec2((p.x + 1) / 2, 1.0-(p.y + 1) / 2);
	else u = vec2((p.x + 1) / 2, (p.y + 1) / 2);
	gl_Position = vec4(p.x, p.y, 1, 1);
}