#version 330

in vec3 fragNormal;
in vec3 fragPos;
in vec2 texCoord;
in float fragWaterHeight;

float minWaterHeight = 0.0;
float minWaterOpacity = 0.2;

float maxWaterHeight = 5.0;
float maxWaterOpacity = 0.8;

out vec4 fragColor;

uniform sampler2D texture0;

void main()
{
	if (fragWaterHeight < minWaterHeight)
	{
		fragColor = vec4(0);
		return;
	}
	float a = smoothstep(minWaterHeight, maxWaterHeight, fragWaterHeight);
	float b = mix(minWaterOpacity, maxWaterOpacity, a);
	fragColor = vec4(15.0 / 256, 94.0 / 256, 156.0 / 256.0, b);
}