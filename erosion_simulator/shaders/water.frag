#version 330

#define PI 3.145926538

in vec3 fragNormal;
in vec3 fragPos;
in vec2 texCoord;
in float fragWaterHeight;
in vec2 fragWaterVelocity;
in float fragSediment;


float minWaterHeight = 0.15;
float minWaterOpacity = 0.3;

float maxWaterHeight = 5.0;
float maxWaterOpacity = 0.8;

out vec4 fragColor;

uniform vec3 viewerPosition;
uniform sampler2D texture0;
uniform float deltaTime;
uniform int waterDebugMode;


vec3 lightDirection = normalize(vec3(1.0,10.0, 3.0));

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

float getRed(float velocity)
{
	if(velocity < 0.5)
		return 0;
	else
	{
		return (velocity - 0.25) * (4/3);
	}
}

float getBlue(float velocity)
{
	if(velocity > 0.5)
		return 0;
	else
		return 1 - (3/4) * velocity;

}

void main()
{
	if(fragWaterHeight < minWaterHeight){
		fragColor = vec4(0);
		return;
	}

	float velocity = length(fragWaterVelocity);
	// water velocity
	if(waterDebugMode == 1) {
		fragColor = vec4(getRed(velocity), getGreen(velocity), getBlue(velocity), 1);
		return;
	}
	// sediment transportation
	else if (waterDebugMode == 2)
	{
		fragColor = vec4(3 * fragSediment, 0,0,1);
		return;
	}
	// invisible
	else if( waterDebugMode == 3)
	{
		fragColor = vec4(0);
		return;
	}

	vec3 baseColor = vec3(15.0 / 256, 94.0 / 256, 156.0 / 256.0);

	float a = smoothstep(minWaterHeight, maxWaterHeight, fragWaterHeight);
	float alpha = mix(minWaterOpacity, maxWaterOpacity, a);

	vec3 normalMap = texture(texture0, texCoord).rbg * 2.0 - 1.0;
	vec3 normal = normalize(fragNormal + normalMap);

	float diffuseFactor = max(dot(normal, lightDirection), 0.0);
	vec4 diffuseColor = vec4(vec3(diffuseFactor), 1.0);
	
	vec3 viewingRay = normalize(viewerPosition - fragPos);
	vec3 reflectedvertex = reflect(-lightDirection, normal);		
	float specularFactor = pow(max(dot(viewingRay, reflectedvertex),0), 256);
	vec4 specularColor = vec4(0.5 * vec3(specularFactor), 1.0);
	

	baseColor -= vec3(0.1 * min(1, velocity));
	vec4 base = vec4(baseColor, 1.0);

	fragColor = clamp(base *(diffuseColor + specularColor), 0, 1);
	fragColor.a = alpha;
	
}

