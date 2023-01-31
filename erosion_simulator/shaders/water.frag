#version 330

#define PI 3.145926538

in vec3 fragNormal;
in vec3 fragPos;
in vec2 texCoord;
in float fragWaterHeight;
in vec2 fragWaterVelocity;


float minWaterHeight = 0.05;
float minWaterOpacity = 0.2;

float maxWaterHeight = 5.0;
float maxWaterOpacity = 0.6;

out vec4 fragColor;

uniform vec3 viewerPosition;
uniform sampler2D texture0;
uniform float deltaTime;

vec3 lightDirection = normalize(vec3(1.0,-3.0,0.0));
vec3 lightPos = vec3(100.0,1000.0,-100.0);

float getGreen(float velocity)
{
	float g = 0;
	if(velocity < 0.5)
	{
		g = velocity * 2;
	}
	else if (velocity > 0.5)
	{
		g = 1 - velocity * 2;
	}
	else
	{
		g = 1;
	}

	return g;
}

void main()
{
	vec3 baseColor = vec3(15.0 / 256, 94.0 / 256, 156.0 / 256.0);

	if (fragWaterHeight < minWaterHeight)
	{
		fragColor = vec4(0);
		return;
	}

	float a = smoothstep(minWaterHeight, maxWaterHeight, fragWaterHeight);
	float alpha = mix(minWaterOpacity, maxWaterOpacity, a);

	float diffuseFactor = max(dot(fragNormal, - normalize(viewerPosition - lightPos)), 0.0);
	vec4 diffuseColor = vec4(vec3(1.0, 1.0, 1.0) * 1.0 * diffuseFactor, 1.0);

	vec4 specularColor = vec4(0.0);

	if(diffuseFactor > 0.0) 
	{
		vec3 viewingRay = normalize(viewerPosition - fragPos);
		vec3 reflectedvertex = reflect(-normalize(viewerPosition - lightPos), normalize(fragNormal));
		
		float specularFactor = dot(viewingRay, reflectedvertex);
		if(specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, 20);
			specularColor = vec4(vec3(250.0 / 255.0, 214.0 / 256.0, 165.0/256.0) * 1 * specularFactor, 1.0);
		}
	}

	//fragColor = clamp(vec4(baseColor, 1.0) * (diffuseColor + specularColor), vec4(0), vec4(1));
	//fragColor.a = alpha;

	//debug water velocity
	float velocity = min(1, length(fragWaterVelocity));
	fragColor = vec4(velocity, getGreen(velocity), 1 - velocity, 1);


}

