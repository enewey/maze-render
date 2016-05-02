#version 410 core

uniform vec3 pos;
uniform float scalar;
uniform float reflecting;

layout(std140) uniform constants
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

out vec2 tc;

void main(void) {
	vec2[6] quad = vec2[6](vec2(-1.0, -1.0),
							vec2(1.0, -1.0),
							vec2(1.0, 1.0),

							vec2(1.0, 1.0),
							vec2(-1.0, 1.0),
							vec2(-1.0, -1.0)
						);

	vec2[6] tcquad =  vec2[6](vec2(0.0, 1.0),
							  vec2(1.0, 1.0),
							  vec2(1.0, 0.0),

							  vec2(1.0, 0.0),
							  vec2(0.0, 0.0),
							  vec2(0.0, 1.0)
						);

	
	vec4 P = mv_matrix * vec4(pos, 1.0);
	P.xy = P.xy + (quad[gl_VertexID] * scalar);
	tc = tcquad[gl_VertexID].xy;

	if (reflecting < 0) {
		tc.y = -tc.y;
	}

	gl_Position = proj_matrix * P;
}

