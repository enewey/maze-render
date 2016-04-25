#version 410 core

layout (location = 0) in vec4 position;                            
layout (location = 1) in vec4 normal;
layout (location = 2) in vec4 pos_color; 
																	
layout(std140) uniform constants									
{																	
	mat4 mv_matrix;													
	mat4 view_matrix;												
	mat4 proj_matrix;												
	vec4 ball_color;
};	

uniform int settings[4];

uniform vec3 light_pos = vec3(5.0, 15.0, 0.0);

// Outputs to Fragment Shader
out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec3 color;
} vs_out;

void main(void)
{
    // Calculate view-space coordinate
    vec4 P = mv_matrix * position;

    // Calculate normal in view-space
	vs_out.N = mat3(mv_matrix) * -position.xyz;

    // Calculate light vector
    vs_out.L = (light_pos - P.xyz)  * mat3(mv_matrix);

    // Calculate view vector
    vs_out.V = -P.xyz;

	vs_out.color = ball_color.rgb;

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}