
#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

void main()
{
	vec4 color = texture(tex1,texCoord);
	outFragColor = color;
}

