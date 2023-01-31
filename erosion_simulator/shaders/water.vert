#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in float height;
layout (location = 4) in vec2 velocity;

out vec3 fragNormal;
out vec3 fragPos;
out vec2 texCoord;
out float fragWaterHeight;
out vec2 fragWaterVelocity;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main()
{
	gl_Position = projection * view * model * vec4(pos.x, pos.y + height, pos.z, 1.0);
	
	fragNormal = mat3(transpose(inverse(model))) * normal;
	
	fragPos = (model * vec4(pos, 1.0)).xyz;
	
	texCoord = uv;

	fragWaterHeight = height;

	fragWaterVelocity = velocity;
}