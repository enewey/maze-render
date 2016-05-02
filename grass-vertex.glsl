#version 410 core

layout (location = 0) in vec3 position;

layout(std140) uniform constants
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

uniform vec4 light_pos;
uniform float reflecting;

out VS_OUT {
	vec3 L;
    vec3 V;
	vec3 N;
	vec2 tc;
	float light_dist;
	float attenuation;
} vs_out;

uniform float constant_att  = 0.04; 
uniform float linear_att    = 0.10; 
uniform float quadratic_att = 0.015;

void main(void) {
	float scalar = 0.1; //size of texture
	vec2[6] quad = vec2[6](vec2(-1.0, 1.0), //0,0
						   vec2(-1.0, -1.0),
						   vec2(1.0, -1.0),
						   vec2(1.0, -1.0),
						   vec2(1.0, 1.0),
						   vec2(-1.0, 1.0));

	vec2[6] tcquad = vec2[6](vec2(0.0, 0.0),
							 vec2(0.0, 1.0),
							 vec2(0.5, 1.0),
							 vec2(0.5, 1.0),
							 vec2(0.5, 0.0),
							 vec2(0.0, 0.0));

	float tc_u = 0.5;
	if (gl_VertexID % 12 > 5) {
		tc_u = 0.0;
	}

	vec4 P = mv_matrix * vec4(position, 1.0);
	vec3 L = (mv_matrix*light_pos).xyz - P.xyz;
	float d = distance(P.xyz, L);
	vs_out.light_dist = d;

	vs_out.attenuation = 1.0 / ( constant_att +	(linear_att*d) + (quadratic_att*d*d));

	int modu = gl_VertexID % 6;
	P.xy = P.xy + (quad[modu] * scalar) - vec2(0.0, 1.0-scalar); //add scaled vertex to form triangle

	vec3 vout = -P.xyz;
	vec2 tcout = vec2(tcquad[modu].x + tc_u, tcquad[modu].y);
	if (reflecting < 0) {
		vout.y = -vout.y;
		tcout.y = -tcout.y;
	}
	
	vs_out.V = vout;
	vs_out.tc = tcout;
	vs_out.L = L;
	vs_out.N = vec3(0.0, 0.0, 1.0);
	gl_Position = proj_matrix * P; //finally put into proper space
}