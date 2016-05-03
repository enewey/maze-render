#version 410 core

uniform sampler2D tex;
uniform sampler2D bump_map;

// Output
out vec4 color;

in VS_OUT
{
	mat4 mv_matrix;
    vec3 L;
    vec3 V;
	vec2 tc;
	float light_dist;
	float attenuation;
	float reflecting;
} fs_in;

// Material properties
uniform vec3 diffuse_color = vec3(0.3, 0.3, 0.3);
uniform vec3 specular_color = vec3(0.5);
uniform float specular_power = 10.0;
uniform vec3 ambient = vec3(0.03, 0.03, 0.03);

void main(void)
{
	vec3 bumpN = (texture(bump_map, fs_in.tc)).xyz;// vec2(fs_in.tc.x, fs_in.tc.y * fs_in.reflecting))).xyz;
	bumpN = (bumpN-0.5)*2.0;

	vec3 testV = fs_in.V;
	if (fs_in.reflecting < 0) {
		bumpN.y = -bumpN.y;
		testV.y = -testV.y;
	}


	// Normalize the incoming N, L and V vectors
	
    vec3 N = normalize(bumpN);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(testV);
	
	// Calculate R locally
    vec3 R = normalize(-reflect(L, N));
	
    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_color  * fs_in.attenuation;
    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_color  * fs_in.attenuation;

    // Write final color to the framebuffer
    color = vec4((ambient + diffuse + specular), 1.0) * texture(tex, fs_in.tc);
}