#version 410 core

layout (location = 0) in vec4 position;                            
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;
																	
layout(std140) uniform constants									
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

uniform vec4 light_pos;

// Outputs to Fragment Shader
out VS_OUT
{
	mat4 mv_matrix;
    vec3 L;
    vec3 V;
	vec2 tc;
	float light_dist;
	float attenuation;
} vs_out;

uniform float constant_att  = 0.05; 
uniform float linear_att    = 0.10; 
uniform float quadratic_att = 0.015;

void main(void)
{
	vs_out.mv_matrix = mv_matrix;

    vec4 P = mv_matrix * position;
    vec3 L = (mv_matrix*light_pos).xyz - P.xyz;

	float d = distance(P.xyz, L);

	vs_out.light_dist = d;
	vs_out.attenuation = 1.0 / ( constant_att +	(linear_att*d) + (quadratic_att*d*d));
	vs_out.L = L;
    vs_out.V = -P.xyz;
	vs_out.tc = tex_coord;

    gl_Position = proj_matrix * P;
}