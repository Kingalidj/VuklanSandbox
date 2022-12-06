#version 450

#include "res/shaders/random.glsl"

layout (location = 0) in vec3 vPosition;
//layout (location = 1) in vec4 vColor;
//layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 viewProj;
} cameraData;

vec2 random_vec2(float c) {
	float x = random(c) * 2 - 1;
	float y = random(random(x)) * 2 - 1;
	return vec2(x, y);
}

#define SIZE 100

void main()
{
	vec3 dp = vec3(random_vec2(gl_InstanceIndex), 0) * SIZE;
	dp.z = -random(dp.x);

	vec3 pos = vPosition + dp;
	//pos -= vec3(0, 0, random(gl_InstanceIndex) * 2);

	mat4 transformMatrix = cameraData.viewProj;
	gl_Position = transformMatrix * vec4(pos, 1.0f);


	float cr = random(gl_InstanceIndex);
	float cg = random(cr);
	float cb = random(cg);

	outColor = vec4(cr, cg, cb, 1.0);
}


