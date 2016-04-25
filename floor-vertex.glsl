#version 410 core

layout (location = 0) in vec4 position;                            
//layout (location = 1) in vec4 normal;
//layout (location = 2) in vec2 tex_coord;
																	
layout(std140) uniform constants									
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

uniform vec3 light_pos = vec3(20.0, 20.0, -20.0);

// Outputs to Fragment Shader
out VS_OUT
{
	mat4 mv_matrix;
    //vec3 N;
    vec3 L;
    vec3 V;
	vec2 tc;
} vs_out;

void main(void)
{
	vs_out.mv_matrix = mv_matrix;

    // Calculate view-space coordinate
    vec4 P = mv_matrix * position;

	//vs_out.N = mat3(mv_matrix) * normal.xyz;

    // Calculate light vector
    vs_out.L = (light_pos - P.xyz);//mat3(mv_matrix) * 

    // Calculate view vector
    vs_out.V = -P.xyz;

	vs_out.tc = position.xz;

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}