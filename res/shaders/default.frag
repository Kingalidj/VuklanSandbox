#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) flat in int texID;
layout (location = 3) in float radius;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D textures[25];

void main()
{
	vec2 center = texCoord * 2 - vec2(1, 1);
	float d = center.x * center.x + center.y * center.y;
	
	if (d <= radius) {
		outFragColor = texture(textures[texID], texCoord) * inColor;
	} else {
		outFragColor = vec4(0, 0, 0, 0);
	}
}


