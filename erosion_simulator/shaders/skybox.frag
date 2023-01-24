#version 330
in vec3 TexCoord;

out vec4 color;

uniform samplerCube skybox;

struct DirectionalLight
{
	vec3 colour; 
	float ambientIntensity;
	vec3 direction;
	float diffuseIntensity;
};

uniform DirectionalLight directionalLight;

void main()
{
	color =  texture(skybox, TexCoord);
}