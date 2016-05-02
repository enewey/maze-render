#version 410 core

uniform sampler2D tex;

in vec2 tc;
out vec4 color;

void main(void) {
	color = texture(tex, tc);
}