#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;

int threshold1 = 15;
int threshold2 = 30;
int threshold3 = 90;

vec3 lightDirection = normalize(vec3(1.0,-5.0,0.0));

void main()
{
	// check slope angle and color
	float theta = degrees(acos(dot(fragNormal, vec3(0.0, 1.0, 0.0))));

	vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);

	vec4 color1 = texture(texture0, texCoord);
	vec4 color2 = texture(texture1, texCoord);
	vec4 color3 = texture(texture2, texCoord);
	
	if(theta < threshold1) 
	{
		float a = smoothstep(0, threshold1, theta);
		baseColor = vec4(mix(color1, color2, a));
	} 
	else if (theta < threshold2)
	{
		float a = smoothstep(threshold1, threshold2, theta);
		baseColor = vec4(mix(color2, color3, a));
	}
	else if (theta < threshold3)
	{
		baseColor = color3;
	}

	vec4 ambientColor = vec4(vec3(1.0, 1.0, 1.0) * 1 * baseColor.xyz, 1.0);

	float diffuseFactor = max(dot(fragNormal, - lightDirection), 0.0);
	vec4 diffuseColor = vec4(vec3(1.0, 1.0, 1.0) * 1.0 * diffuseFactor, 1.0);
	FragColor = ambientColor * diffuseColor;
}