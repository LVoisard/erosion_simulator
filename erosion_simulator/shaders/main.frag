#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoord;
in float fragOriginalHeight;

out vec4 FragColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;

int grass = 25;
int threshold1 = 45;
int rock = 90;

vec3 lightDirection = normalize(vec3(0.0,-1.0,0.0));

uniform vec3 cursorOverTerrainPos;
uniform float brushRadius;

uniform int debugMode;

void main()
{
	// debug modes

	// show how prone the terrain is to being erroded by force-based erosion
	if(debugMode == 1)		
	{
		float tiltAngle = acos(dot(fragNormal, vec3(0, 1, 0)));
		float terrainProne = max(sin(tiltAngle), 0.05);
		FragColor = vec4(terrainProne, 0, 0, 1);
		return;
	} 
	else if (debugMode == 2)
	{
		float sedimentDeposited = fragPos.y - fragOriginalHeight;
		if(sedimentDeposited > 0)
			FragColor = vec4(0,0,min(sedimentDeposited, 1),1);
		else
			FragColor = vec4(min(abs(sedimentDeposited), 1),0,0,1);
		return;
	}
	else if (debugMode == 3)
	{
		FragColor = vec4(0);
		return;
	}


	// check slope angle and color
	float theta = degrees(acos(dot(fragNormal, vec3(0.0, 1.0, 0.0))));

	vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);

	vec4 color1 = texture(texture0, texCoord);
	vec4 color2 = texture(texture1, texCoord);
	vec4 color3 = texture(texture2, texCoord);

	float heigthDiff = smoothstep(0.0, 1.0, abs(fragOriginalHeight - fragPos.y));
	
	
	if(theta < grass)
	{			
		baseColor = color1;
	}
	else if(theta < threshold1)
	{
		float a = smoothstep(grass, threshold1, theta);
		baseColor = mix(color1, color3, a);
	}
	else if (theta < rock)
	{
		baseColor = color3;
	}	
	baseColor = mix(baseColor, color2, heigthDiff);

	vec4 ambientColor = vec4(vec3(1.0, 1.0, 1.0) * 1 * baseColor.xyz, 1.0);

	float diffuseFactor = max(dot(fragNormal, - lightDirection), 0.0);
	vec4 diffuseColor = vec4(vec3(1.0, 1.0, 1.0) * 1.0 * diffuseFactor, 1.0);

	FragColor = ambientColor * diffuseColor;
	if(length((fragPos - cursorOverTerrainPos)) < brushRadius) {
		FragColor = clamp(FragColor + vec4(0.25), vec4(0.0), vec4(1.0));
	}


}