#version 410 core

// Code is modified from OpenGL SuperBible example

// Per-vertex inputs
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

// Matrices we'll need
layout (std140) uniform constants
{
    mat4 mv_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
};

// Outputs to Fragment Shader
out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec3 color;
} vs_out;

// Position of light
uniform vec3 light_pos = vec3(0.0,0.0,0.0);
uniform int oneColor = 0;

void main(void)
{
    // Calculate view-space coordinate
    vec4 P = mv_matrix * position;

    // Calculate normal in view-space
    vs_out.N = mat3(mv_matrix) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector
    vs_out.V = -P.xyz;

	if(oneColor == 0)
	{
		vs_out.color = vec3(position * 2.0 + vec4(0.5, 0.5, 0.5, 0.0));
	}
	else
	{
		vs_out.color = vec3(0.839, 0.753, 0.69);
	}

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}
