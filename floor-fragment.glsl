#version 410 core

uniform sampler2D bump_map;

// Output
out vec4 color;

in VS_OUT
{
	mat4 mv_matrix;
	//vec3 N;
    vec3 L;
    vec3 V;
    vec2 tc;
} fs_in;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.7, 0.5, 0.3);
uniform vec3 ambient = vec3(0.1, 0.1, 0.1);

void main(void)
{
	
	vec3 bumpN = (fs_in.mv_matrix * texture(bump_map, fs_in.tc)).xyz;
	bumpN = bumpN.xzy;
	bumpN.y = -bumpN.y;

	// Normalize the incoming N, L and V vectors
    vec3 N = normalize((bumpN-0.5)*2.0);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

	// Calculate R locally
    vec3 R = reflect(-L, N);

    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;

    // Write final color to the framebuffer
    color = vec4((ambient + diffuse), 1.0); //* texture(bump_map, fs_in.tc);

}