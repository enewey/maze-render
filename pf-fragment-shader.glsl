#version 410 core

// Code is modified from OpenGL SuperBible example

// Output
out vec4 color;

// Input from vertex shader
in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec3 color;
} fs_in;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.5, 0.5, 0.5);
uniform vec3 specular_albedo = vec3(0.9);
uniform float specular_power = 256.0;
uniform vec3 ambient = vec3(0.2, 0.2, 0.2);

void main(void)
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R locally
    vec3 R = reflect(-L, N);

    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

    // Write final color to the framebuffer
    color = vec4((ambient + diffuse + specular)*fs_in.color, 1.0);
}
