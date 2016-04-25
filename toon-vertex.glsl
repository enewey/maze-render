#version 410 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

layout(std140) uniform constants									
{																	
	mat4 mv_matrix;													
	mat4 view_matrix;												
	mat4 proj_matrix;												
	vec4 ball_color;
};


uniform vec3 light_pos = vec3(5.0, 15.0, 0.0);

out VS_OUT
{
    vec3 N;
    vec3 V;
	vec3 L;
} vs_out;

void main(void)
{
    vec4 P = mv_matrix * position;

    // Calculate eye-space normal and position
    vs_out.N = mat3(mv_matrix) * -position.xyz;
    vs_out.V = -P.xyz;
	vs_out.L = (light_pos - P.xyz)  * mat3(mv_matrix);

    // Send clip-space position to primitive assembly
    gl_Position = proj_matrix * P;
}
