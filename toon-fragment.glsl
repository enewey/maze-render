#version 410 core

//uniform sampler1D tex_toon;


in VS_OUT
{
    vec3 N;
    vec3 V;
	vec3 L;
} fs_in;

out vec4 color;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.5, 0.5, 0.5);
uniform vec3 specular_albedo = vec3(0.9);
uniform float specular_power = 50.0;
uniform vec3 ambient = vec3(0.2, 0.2, 0.2);

uniform vec3 bcolor = vec3(1.0f, 0.1f, 0.5f);

void main(void)
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R locally
    vec3 R = reflect(-L, N);

    // Compute the diffuse and specular components for each fragment
	float toonF = round(max(dot(N, L), 0.0) * 3.0) / 3.0;
    vec3 diffuse = toonF * diffuse_albedo;

	float toonS = step(0.95, max(dot(R, V), 0.0));
    vec3 specular = pow(toonS, specular_power) * specular_albedo;

    // Write final color to the framebuffer
    color = vec4((ambient + diffuse + specular) * bcolor, 1.0);
}