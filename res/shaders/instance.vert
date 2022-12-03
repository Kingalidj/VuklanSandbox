#version 450

layout (location = 0) in vec3 vPosition;
//layout (location = 1) in vec4 vColor;
//layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 viewProj;
} cameraData;

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}



// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }



// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}



// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

vec2 random_vec2(float c) {
	float x = random(c) * 2 - 1;
	float y = random(random(x)) * 2 - 1;
	return vec2(x, y);
}

#define SIZE 1000

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


