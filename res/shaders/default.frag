#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float texIndx;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D tex[2];

void main()
{
	vec4 tex0 = texture(tex[0], texCoord);
	vec4 tex1 = texture(tex[1], texCoord);

	outFragColor = tex0 * texIndx + tex1 * (1-texIndx);
}

