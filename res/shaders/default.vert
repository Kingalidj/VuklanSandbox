#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out float texIndx;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 viewProj;
} cameraData;

void main()
{
	mat4 transformMatrix = cameraData.viewProj;
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);

	texIndx = gl_VertexIndex % 2;

	outColor = vColor;
	texCoord = vTexCoord;
}

