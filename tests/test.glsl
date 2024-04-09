/* Auto-generated with Radeon GPU Analyzer (RGA).*/
#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

float TestFunc(float v)
{
	return log2(v-4.0f);
}

void main()
{
 	float f =TestFunc(fragColor.r) + 123.05f;
    outColor = vec4(f, 0.0f, 0.0f, 0.0f);
}
