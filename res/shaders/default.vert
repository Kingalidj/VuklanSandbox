#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 texCoord;

void main()
{
	gl_Position = vec4(vPosition, 1.0f);
	outColor = vColor;
	texCoord = vTexCoord;
}
