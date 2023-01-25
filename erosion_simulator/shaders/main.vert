#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	fragPos = pos;
	fragNormal = normal;
	texCoord = uv;
	gl_Position = projection * view * model * vec4(pos, 1.0);
}