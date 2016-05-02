#version 410 core

uniform sampler2D floor_tex;
uniform vec2 window_size;
uniform float curr_time;

// Output
out vec4 color;

in vec4 pos;

uniform float freq = 60.0;
uniform float amp = 0.005;
uniform float wave_speed = 0.05;

void main(void)
{
	vec2 tc = vec2(gl_FragCoord.x / window_size.x, gl_FragCoord.y / window_size.y);

	float wave = wave_speed * curr_time;
	float displacement = (sin((tc.y + wave) * freq) * amp);

	tc.x = tc.x + displacement;
	vec4 tex_color = texture(floor_tex, tc);


	color = vec4( ((tex_color * vec4(0.9, 0.9, 1.0, 1.0) + vec4(0.1, 0.1, 0.25, 1.0)) / 1.5).xyz, 1.0);
}