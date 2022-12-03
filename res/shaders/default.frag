#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D tex;

void main()
{
	outFragColor = texture(tex, texCoord);
}

