#version 410 core

uniform sampler2D grass;

in VS_OUT {	
	vec3 L;	
    vec3 V;
	vec3 N;
	vec2 tc;
	float light_dist;
	float attenuation;
} fs_in;

out vec4 color;

// Material properties
uniform vec3 diffuse_color = vec3(0.3, 0.25, 0.3);
uniform vec3 specular_color = vec3(0.5);
uniform float specular_power = 10.0;
uniform vec3 ambient = vec3(0.03, 0.03, 0.03);

void main(void)
{
	vec4 tcolor = texture(grass, fs_in.tc);
	if (tcolor.r == 0.0 && tcolor.g == 0.0 && tcolor.b == 0.0) { discard; }

	// Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
	
	// Calculate R locally
    vec3 R = normalize(-reflect(L, N));
	
    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_color  * fs_in.attenuation;
    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_color  * fs_in.attenuation;

    color = vec4(tcolor.rgb * vec3(diffuse + specular + ambient), 1.0);
}