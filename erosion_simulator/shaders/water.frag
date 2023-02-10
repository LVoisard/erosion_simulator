#version 330

#define PI 3.145926538

in vec3 fragNormal;
in vec3 fragPos;
in vec2 texCoord;
in float fragWaterHeight;
in vec2 fragWaterVelocity;


float minWaterHeight = 0.15;
float minWaterOpacity = 0.3;

float maxWaterHeight = 5.0;
float maxWaterOpacity = 0.8;

out vec4 fragColor;

uniform vec3 viewerPosition;
uniform sampler2D texture0;
uniform float deltaTime;
uniform int debugWaterVelocity;

vec3 lightDirection = normalize(vec3(0.0,-3.0,0.0));

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

	float diffuseFactor = max(dot(fragNormal, -lightDirection), alpha);
	vec4 diffuseColor = vec4(vec3(1.0, 1.0, 1.0) * 1.0 * diffuseFactor, 1.0);

	vec4 specularColor = vec4(0.0);

	if(diffuseFactor > 0.0) 
	{
		vec3 viewingRay = normalize(viewerPosition - fragPos);
		vec3 reflectedvertex = reflect(-lightDirection, normalize(fragNormal));
		
		float specularFactor = dot(viewingRay, reflectedvertex);
		if(specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, 100);
			specularColor = vec4(vec3(1) * 1 * specularFactor, 1.0);
		}
	}

	float velocity = length(fragWaterVelocity);
	if(debugWaterVelocity == 1) {
		fragColor = vec4(velocity, getGreen(velocity), 1 - velocity, 1);
	}
	else
	{
		vec4 base = vec4(baseColor, 1.0);
		if(fragWaterHeight < minWaterHeight + 0.005)
			base += vec4(1) * (1 - alpha);
		fragColor = clamp(base * (diffuseColor + specularColor), vec4(0), vec4(1));
		fragColor.a = alpha;
	}
}

