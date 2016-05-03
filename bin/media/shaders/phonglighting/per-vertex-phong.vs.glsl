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

// Light and material properties
uniform vec3 light_pos = vec3(0.0, 0.0, 0.0);
uniform vec3 diffuse_albedo = vec3(0.5, 0.2, 0.7);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 128.0;
uniform vec3 ambient = vec3(0.2, 0.2, 0.2);
uniform int oneColor = 0;

// Outputs to the fragment shader
out VS_OUT
{
    vec3 color;
} vs_out;

void main(void)
{
    // Calculate view-space coordinate
    vec4 P = mv_matrix * position;

    // Calculate normal in view space
    vec3 N = mat3(mv_matrix) * normal;
    // Calculate view-space light vector
    vec3 L = light_pos - P.xyz;
    // Calculate view vector (simply the negative of the view-space position)
    vec3 V = -P.xyz;

    // Normalize all three vectors
    N = normalize(N);
    L = normalize(L);
    V = normalize(V);
    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    // Calculate the diffuse and specular contributions
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

	// Send output color to fragment shader
	if(oneColor == 0)
	{
		vec3 tempColor = vec3(position * 2.0 + vec4(0.5, 0.5, 0.5, 0.0));
		vs_out.color = (ambient + diffuse + specular)*tempColor;
	}
	else
	{
		vs_out.color = (ambient + diffuse + specular)*vec3(.839, .753, .69);
	}

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}
