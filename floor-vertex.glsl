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

out vec4 pos;

void main(void)
{
	pos = proj_matrix * mv_matrix * position;
    gl_Position = pos;
}