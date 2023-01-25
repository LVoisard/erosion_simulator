#version 330

in vec3 fragNormal;
in vec3 fragPos;
in vec2 texCoord;

// input this per vertex
int waterHeight = 1;

out vec4 fragColor;

uniform sampler2D texture0;

void main()
{	
	fragColor = vec4(15.0 / 256, 94.0 / 256, 156.0 / 256.0, 0.2);
}