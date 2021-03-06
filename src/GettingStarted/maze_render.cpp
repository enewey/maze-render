#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <assert.h>

#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "../lodepng.h"

#define PI 3.14159265

class maze_render_app : public sb7::application
{

protected:

	void init()
	{
		static const char title[] = "Final Project: Erich Newey";

		sb7::application::init();

		memcpy(info.title, title, sizeof(title));
		info.windowWidth = 1920;
		info.windowHeight = 1080;
	}

	//Functions
	void startup();
	void render(double currentTime);
	void onKey(int key, int action);
	void onMouseMove(int x, int y);
	void onMouseButton(int button, int action);
	vmath::vec3 getArcballVector(int x, int y);
	//void change_settings(GLint n, GLint s, GLint p, GLint c);
	void load_image(std::string filename, GLuint * buf);

	//Load an object file into a single mesh
	bool load_object(std::string filename,	std::vector<vmath::vec4> & out_vertices,
											std::vector < vmath::vec2 > & out_uvs,
											std::vector < vmath::vec3 > & out_normals);

	void generate_grass(int** level, int width, int height, std::vector< vmath::vec3 > & out_grass);
	//Load .mdf (maze data file) into a 2D array of walls and floors (1 == wall, 0 == floor)
	int ** load_level(std::string filename, int & width, int & height, int &startr, int &startc, int &endr, int &endc);
	//Convert an array index into a vertex in the maze
	float convert_to_vert(int coord, int dim);
	int convert_to_coord(float pos, int dim);

	void wall_collision(float & xPos, float & zPos, vmath::vec3 direction, int height, int width, int** level);
	bool load_shader(GLuint & prog, char* vert, char*frag);

	//Programs
	GLuint			walls_program;
	GLuint			grass_program;
	GLuint			floor_program;
	GLuint			sprite_program;
	
	//Uniforms
	struct uniforms_block
	{
		vmath::mat4     mv_matrix;
		vmath::mat4     view_matrix;
		vmath::mat4     proj_matrix;
		vmath::vec4		ball_color;
	};
	GLuint          uniforms_buffer;

	// Rotation and Translation matricies for moving the camera by mouse interaction.
	vmath::mat4 rotationMatrix = vmath::mat4::identity();
	vmath::mat4 translationMatrix = vmath::mat4::identity();

private:

	bool * dirPress;
	float lookingAngle = 0.0f;
	vmath::vec3 direction;
	vmath::vec3 position;

	//"Character" position
	float cXpos;
	float cYpos = 0.0f;
	float cZpos;

	//Trophy position
	float endXpos, endZpos;

	//Where the light sits in the y-axis
	float lightY = 1.0f;

	//Necessary to throttle movement
	double timeElapsed = 0.0;

	int _width, _height, _startr, _startc, _endr, _endc;
	int ** _level;
	
	//Vertex array objects
	GLuint vao2;
	GLuint grass_vao;
	GLuint floor_vao;

	//Texture buffers
	GLuint wall_tex_buffer;
	GLuint wall_normal_buffer;
	GLuint floor_normal_buffer;
	GLuint grass_tex;
	GLuint frame_tex;
	GLuint trophy_tex;

	//Vertex buffers
	GLuint buffer;
	GLuint normal_buffer;
	GLuint tc_buffer;
	std::vector< vmath::vec4 > vertices;
	std::vector< vmath::vec3 > normals;
	std::vector< vmath::vec2 > uvs;

	GLuint fbuffer;
	GLuint fnormal_buffer;
	GLuint ftc_buffer;
	std::vector< vmath::vec4 > fvertices;
	std::vector< vmath::vec3 > fnormals;
	std::vector< vmath::vec2 > fuvs;

	GLuint grass_buffer;
	std::vector< vmath::vec3 > grass_points;

	//Framebuffer objects
	GLuint frame_buf;
	GLuint render_buf;

	int viewport_w = 1920, viewport_h = 1080;

	//Each floor will have n^2 blades of grass
	int grass_blades = 6;

	//Scale of the trophy sprite
	float trophy_scale = 0.5f;
};

void load_vertex(GLuint &buf, GLsizeiptr size, const void * points) {
	glGenBuffers(1, &buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBufferData(GL_ARRAY_BUFFER,
		size,
		points,
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void maze_render_app::startup()
{
	dirPress = new bool[6];//w, d, s, a, right, left
	for (int i = 0; i < 6; i++)
		dirPress[i] = false;

	direction = vmath::vec3(0.0f, 0.0f, -1.0f);

#pragma region Load shaders
	walls_program = glCreateProgram(); //Bump-mapped walls
	grass_program = glCreateProgram(); //Grass sprites
	floor_program = glCreateProgram(); //Water floor
	sprite_program = glCreateProgram(); //Trophy

	assert(load_shader(walls_program, "walls-vertex.glsl", "walls-fragment.glsl"));
	assert(load_shader(floor_program, "floor-vertex.glsl", "floor-fragment.glsl"));
	assert(load_shader(grass_program, "grass-vertex.glsl", "grass-fragment.glsl"));
	assert(load_shader(sprite_program, "sprite-vertex.glsl", "sprite-fragment.glsl"));
#pragma endregion

#pragma region Create Framebuffer Object
	glGenFramebuffers(1, &frame_buf);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buf);

	glGenTextures(1, &frame_tex);
	glBindTexture(GL_TEXTURE_2D, frame_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewport_w, viewport_h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame_tex, 0);

	glGenRenderbuffers(1, &render_buf);
	glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewport_w, viewport_h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buf);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE); //make sure FBO is created

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Load Textures
	//Load textures
	load_image("bin\\media\\textures\\wall.png", &wall_tex_buffer);
	load_image("bin\\media\\textures\\normal.png", &wall_normal_buffer);
	load_image("bin\\media\\textures\\floor_normal.png", &floor_normal_buffer);
	load_image("bin\\media\\textures\\grass_tex.png", &grass_tex);
	load_image("bin\\media\\textures\\trophy.png", &trophy_tex);
#pragma endregion

#pragma region Load Object data
	//Object data loaded from files
	bool res = load_object("bin\\media\\objects\\wall_data.obj", vertices, uvs, normals);
	assert(res);
	res = load_object("bin\\media\\objects\\floor_data.obj", fvertices, fuvs, fnormals);
	assert(res);
#pragma endregion

#pragma region Load and initialize level data
	_level = load_level("bin\\media\\objects\\walls.mdf", _width, _height, _startr, _startc, _endr, _endc);

	//Set the starting position
	cXpos = convert_to_vert(_startc, _width) - 1.0f;
	cZpos = convert_to_vert(_startr, _height) - 1.0f;

	//Set the end position (the trophy)
	endXpos = convert_to_vert(_endc, _width) - 1.0f;
	endZpos = convert_to_vert(_endr, _height) - 1.0f;

	//Generate the points for grass sprites
	generate_grass(_level, _width, _height, grass_points);
#pragma endregion
	
#pragma region Wall buffers

	//Cubes vao
	glGenVertexArrays(1, &vao2);
	glBindVertexArray(vao2);

	load_vertex(buffer, vertices.size() * sizeof(vmath::vec4), &vertices[0]);
	load_vertex(normal_buffer, normals.size() * sizeof(vmath::vec3), &normals[0]);
	load_vertex(tc_buffer, uvs.size() * sizeof(vmath::vec2), &uvs[0]);
	
#pragma endregion

#pragma region Floor Buffers
	//Floor vao
	glGenVertexArrays(1, &floor_vao);
	glBindVertexArray(floor_vao);

	load_vertex(fbuffer, fvertices.size() * sizeof(vmath::vec4), &fvertices[0]);
	load_vertex(fnormal_buffer, fnormals.size() * sizeof(vmath::vec3), &fnormals[0]);
	load_vertex(ftc_buffer, fuvs.size() * sizeof(vmath::vec2), &fuvs[0]);
#pragma endregion

#pragma region Grass buffers
	glGenVertexArrays(1, &grass_vao);
	glBindVertexArray(grass_vao);

	load_vertex(grass_buffer, grass_points.size() * sizeof(vmath::vec3), &grass_points[0]);
#pragma endregion

	// Buffer for uniform block
	glGenBuffers(1, &uniforms_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, uniforms_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms_block), NULL, GL_DYNAMIC_DRAW);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}


void maze_render_app::render(double currentTime)
{
	static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
	static const GLfloat skyBlue[] = { 0.529f, 0.808f, 0.922f };
	static const GLfloat ones[] = { 1.0f };
	const float f = (float)currentTime;
	
	vmath::vec3 view_position = vmath::vec3(cXpos, 0.0f, cZpos);
	if (f - timeElapsed > 0.01) {
#pragma region Camera positioning with key detection
		timeElapsed = f;
		if (dirPress[4]) {
			lookingAngle += 2.0f;
		}
		if (dirPress[5]) {
			lookingAngle -= 2.0f;
		}

		rotationMatrix = vmath::rotate(lookingAngle, vmath::vec3(0.0f, 1.0f, 0.0f));
		vmath::vec4 dir = vmath::vec4(direction[0], direction[1], direction[2], 1.0f) * rotationMatrix;
		direction = vmath::vec3(dir[0], dir[1], dir[2]);
		lookingAngle = 0.0f;
		vmath::vec3 strafe = vmath::cross(direction, vmath::vec3(0.0f, 1.0f, 0.0f));

		float movespeed = 0.1f;
		if (dirPress[0]) {
			view_position += (direction * movespeed);
		}
		if (dirPress[1]) {
			view_position += (strafe * movespeed);
		}
		if (dirPress[2]) {
			view_position -= (direction * movespeed);
		}
		if (dirPress[3]) {
			view_position -= (strafe * movespeed);
		}

		cXpos = view_position[0];
		cZpos = view_position[2];
#pragma endregion

#pragma region Collision detection
		wall_collision(cXpos, cZpos, direction, _height, _width, _level);
		view_position[0] = cXpos;
		view_position[2] = cZpos;
#pragma endregion
	}

	vmath::vec3 light_pos = vmath::vec3(view_position[0], lightY, view_position[2]);

	// Set up view and perspective matrix
	vmath::mat4 view_matrix = vmath::lookat(view_position,
		view_position + direction,
		vmath::vec3(0.0f, 1.0f, 0.0f));
	view_matrix *= translationMatrix;

	vmath::mat4 perspective_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);

	uniforms_block* block;
	vmath::mat4 model_matrix;

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buf);

	glViewport(0, 0, viewport_w, viewport_h);
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

#pragma region Wall Reflection Rendering
	glUseProgram(walls_program);

	glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
	glBindTexture(GL_TEXTURE_2D, wall_tex_buffer);
	glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1	
	glBindTexture(GL_TEXTURE_2D, wall_normal_buffer);

	glBindVertexArray(vao2);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, tc_buffer);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//Draw walls
	glUniform4f(glGetUniformLocation(walls_program, "light_pos"), light_pos[0], light_pos[1], light_pos[2], 1.0f);
	glUniform1f(glGetUniformLocation(walls_program, "reflecting"), -1.0f);
	glUniform1f(glGetUniformLocation(walls_program, "time"), currentTime);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	model_matrix =
		vmath::translate(0.0f, -2.0f, 0.0f) *
		vmath::scale(1.0f, -1.0f, 1.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	//change_settings(1, 1, 0, 0);
	glCullFace(GL_BACK);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	//End Walls
#pragma endregion

#pragma region Grass Reflection rendering
	glUseProgram(grass_program);

	glUniform1i(glGetUniformLocation(grass_program, "grass"), 2);
	glActiveTexture(GL_TEXTURE0 + 2); // Texture unit 2
	glBindTexture(GL_TEXTURE_2D, grass_tex);

	glUniform4f(glGetUniformLocation(grass_program, "light_pos"), light_pos[0], light_pos[1], light_pos[2], 1.0f);
	glUniform1f(glGetUniformLocation(grass_program, "reflecting"), -1.0f);

	glBindVertexArray(grass_vao);
	glBindBuffer(GL_ARRAY_BUFFER, grass_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glVertexAttribDivisor(0, 1);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	model_matrix =
		vmath::translate(0.0f, -0.2f, 0.0f) * 
		vmath::scale(1.0f, -1.0f, 1.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDrawArrays(GL_TRIANGLES, 0, grass_points.size());

	glDisable(GL_BLEND);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion

#pragma region Trophy sprite reflection
	glUseProgram(sprite_program);

	glUniform1i(glGetUniformLocation(sprite_program, "tex"), 4);
	glActiveTexture(GL_TEXTURE0 + 4); // Texture unit 4
	glBindTexture(GL_TEXTURE_2D, trophy_tex);

	glUniform3f(glGetUniformLocation(sprite_program, "pos"), endXpos, 0.0f, endZpos);
	glUniform1f(glGetUniformLocation(sprite_program, "scalar"), trophy_scale);
	glUniform1f(glGetUniformLocation(sprite_program, "reflecting"), -1.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	model_matrix =
		vmath::translate(0.0f, -2.0f, 0.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, info.windowWidth, info.windowHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//Draw everything normal now
#pragma region Floor Rendering (water)
	glUseProgram(floor_program);
	
	glUniform1i(glGetUniformLocation(floor_program, "floor_tex"), 3);
	glActiveTexture(GL_TEXTURE0 + 3); // Texture unit
	glBindTexture(GL_TEXTURE_2D, frame_tex);
	
	glBindVertexArray(floor_vao);
	glBindBuffer(GL_ARRAY_BUFFER, fbuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, fnormal_buffer);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, ftc_buffer);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glUniform2f(glGetUniformLocation(floor_program, "window_size"), (float)info.windowWidth, (float)info.windowHeight);
	glUniform1f(glGetUniformLocation(floor_program, "curr_time"), currentTime);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	//model_matrix =
	//	vmath::scale(1.0f);

	block->mv_matrix = view_matrix;// *model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	//change_settings(1, 1, 0, 0);
	glCullFace(GL_FRONT);
	//glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLES, 0, fvertices.size());
	//glDisable(GL_BLEND);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	#pragma endregion

#pragma region Wall Render
	//Walls
	glUseProgram(walls_program);

	glUniform1i(glGetUniformLocation(walls_program, "tex"), 0);
	glUniform1i(glGetUniformLocation(walls_program, "bump_map"), 1);

	glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
	glBindTexture(GL_TEXTURE_2D, wall_tex_buffer);
	glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1	
	glBindTexture(GL_TEXTURE_2D, wall_normal_buffer);

	glBindVertexArray(vao2);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, tc_buffer);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//Draw walls
	glUniform4f(glGetUniformLocation(walls_program, "light_pos"), light_pos[0], light_pos[1], light_pos[2], 1.0f);
	glUniform1f(glGetUniformLocation(walls_program, "reflecting"), 1.0f);
	glUniform1f(glGetUniformLocation(walls_program, "time"), currentTime);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	model_matrix =
		vmath::scale(1.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	//change_settings(1, 1, 0, 0);
	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	//End Walls
#pragma endregion

#pragma region Grass rendering
	glUseProgram(grass_program);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1i(glGetUniformLocation(grass_program, "grass"), 2);
	glActiveTexture(GL_TEXTURE0 + 2); // Texture unit 2
	glBindTexture(GL_TEXTURE_2D, grass_tex);

	glUniform4f(glGetUniformLocation(grass_program, "light_pos"), light_pos[0], light_pos[1], light_pos[2], 1.0f);
	glUniform1f(glGetUniformLocation(grass_program, "reflecting"), 1.0f);

	glBindVertexArray(grass_vao);
	glBindBuffer(GL_ARRAY_BUFFER, grass_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glVertexAttribDivisor(0, 1);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	model_matrix =
		vmath::scale(1.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, grass_points.size());
	glDisable(GL_BLEND);
	//glEnable(GL_DEPTH_TEST);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion

#pragma region Render trophy sprite
	glUseProgram(sprite_program);
	
	glUniform1i(glGetUniformLocation(sprite_program, "tex"), 4);
	glActiveTexture(GL_TEXTURE0 + 4); // Texture unit 4
	glBindTexture(GL_TEXTURE_2D, trophy_tex);

	glUniform3f(glGetUniformLocation(sprite_program, "pos"), endXpos, 0.0f, endZpos);
	glUniform1f(glGetUniformLocation(sprite_program, "scalar"), trophy_scale);
	glUniform1f(glGetUniformLocation(sprite_program, "reflecting"), 1.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	model_matrix =
		vmath::scale(1.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion

}

void maze_render_app::onKey(int key, int action)
{
	// Check to see if key was pressed
	if (action == GLFW_PRESS)
	{
		switch (key) {
			case 'W':
				dirPress[0] = true;
				break;
			case 'D':
				dirPress[1] = true;
				break;
			case 'S':
				dirPress[2] = true;
				break;
			case 'A':
				dirPress[3] = true;
				break;
			case GLFW_KEY_RIGHT:
				dirPress[4] = true;
				break;
			case GLFW_KEY_LEFT:
				dirPress[5] = true;
				break;
			default:
				break;
		}

		//bShiftPressed = true;
	}
	// Check to see if key was released
	if (action == GLFW_RELEASE) {
		switch (key) {
		case 'W':
			dirPress[0] = false;
			break;
		case 'D':
			dirPress[1] = false;
			break;
		case 'S':
			dirPress[2] = false;
			break;
		case 'A':
			dirPress[3] = false;
			break;
		case GLFW_KEY_RIGHT:
			dirPress[4] = false;
			break;
		case GLFW_KEY_LEFT:
			dirPress[5] = false;
			break;
		default:
			break;
		}
	}
}


void maze_render_app::onMouseButton(int button, int action)
{

}

void maze_render_app::onMouseMove(int x, int y)
{

}

// Modified from tutorial at the following website:
// http://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball

vmath::vec3 maze_render_app::getArcballVector(int x, int y) {
	// find vector from origin to point on sphere
	vmath::vec3 vecP = vmath::vec3(1.0f*x / info.windowWidth * 2 - 1.0f, 1.0f*y / info.windowHeight * 2 - 1.0f, 0.0f);
	// inverse y due to difference in origin location on the screen
	vecP[1] = -vecP[1];
	float vecPsquared = vecP[0] * vecP[0] + vecP[1] * vecP[1];
	// solve for vector z component
	if (vecPsquared <= 1)
		vecP[2] = sqrt(1 - vecPsquared);
	else
		vecP = vmath::normalize(vecP);
	return vecP;
}

//Method to load png images from disk to texture
void maze_render_app::load_image(std::string filename, GLuint * tex_buf) {
	std::vector<unsigned char> image;
	unsigned iwidth, iheight;
	unsigned err = lodepng::decode(image, iwidth, iheight, filename);
	if (err != 0) {
		std::cout << "error" << err << ": " << lodepng_error_text(err) << std::endl;
	}
	size_t u2 = 1, v2 = 1;
	while (u2 < iwidth) u2 *= 2;
	while (v2 < iheight) v2 *= 2;
	double u3 = (double)iwidth / u2;
	double v3 = (double)iheight / v2;

	std::vector<unsigned char> tex(u2*v2 * 4);
	for (size_t y = 0; y < iheight; y++)
	for (size_t x = 0; x < iwidth; x++)
	for (size_t c = 0; c < 4; c++) {
		tex[4 * u2*y + 4 * x + c] = image[4 * iwidth*y + 4 * x + c];
	}
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glGenTextures(1, tex_buf);
	glBindTexture(GL_TEXTURE_2D, *tex_buf);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, iwidth, iheight);
	
	glTexSubImage2D(GL_TEXTURE_2D,  // 2D texture
		0,					// Level 0
		0, 0,				// Offset 0, 0
		iwidth, iheight,    // 256 x 256 texels, replace entire image
		GL_RGBA,			// Four channel data
		GL_UNSIGNED_BYTE,   // data type
		&tex[0]);
}

bool maze_render_app::load_shader(GLuint & prog, char* vert_file, char* frag_file) {
	GLuint vert_shader = sb7::shader::load(vert_file, GL_VERTEX_SHADER);
	GLuint frag_shader = sb7::shader::load(frag_file, GL_FRAGMENT_SHADER);
	glAttachShader(prog, vert_shader);
	glAttachShader(prog, frag_shader);
	glLinkProgram(prog);

	glDetachShader(prog, vert_shader);
	glDetachShader(prog, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	GLint success = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	return (success != GL_FALSE);
}

//http://stackoverflow.com/questions/4711238/using-ifstream-as-fscanf
struct chlit
{
	chlit(char c) : c_(c) { }
	char c_;
};

inline std::istream& operator>>(std::istream& is, chlit x)
{
	char c;
	if (is >> c && c != x.c_)
		is.setstate(std::iostream::failbit);
	return is;
}

int ** maze_render_app::load_level(std::string filename, int & width, int & height, int &startr, int &startc, int &endr, int &endc) {
	std::ifstream file(filename);
	int ** level;
	if (file.is_open())
	{
		int r, c;
		file >> width >> height; //first two numbers will be the width and height of maze
		level = new int*[height];
		for (int i = 0; i < height; i++) {
			level[i] = new int[width];
			for (int j = 0; j < width; j++) {
				level[i][j] = 0;
			}
		}

		file >> startr >> startc >> endr >> endc;
		level[startr][startc] = 2; //start point is 2
		level[endr][endc] = 3; //end point is 3

		while (file >> r >> c) {
			level[c][r] = 1;
		}
	}

	return level;
}

//Load vertex data from an .obj file. Only supports v, vt, vn, and f params.
bool maze_render_app::load_object(std::string filename, std::vector<vmath::vec4> & out_vertices,
														std::vector < vmath::vec2 > & out_uvs,
														std::vector < vmath::vec3 > & out_normals) {
	std::ifstream file(filename);
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< vmath::vec4 > temp_vertices;
	std::vector< vmath::vec2 > temp_uvs;
	std::vector< vmath::vec3 > temp_normals;

	if (file.is_open())
	{
		std::string word;
		while (file >> word)
		{
			if (word.compare("v") == 0) {
				float x, y, z;
				file >> x >> y >> z;
				temp_vertices.push_back(vmath::vec4(x, y, z, 1.0f));
			}
			else if (word.compare("vt") == 0) {
				float u, v;
				file >> u >> v;
				temp_uvs.push_back(vmath::vec2(u,v));
			}
			else if (word.compare("vn") == 0) {
				float x, y, z;
				file >> x >> y >> z;
				temp_normals.push_back(vmath::vec3(x, y, z));
			}
			else if (word.compare("f") == 0) {
				int v, t, n;
				file >> v >> chlit('/') >> t >> chlit('/') >> n;
				vertexIndices.push_back(v);
				uvIndices.push_back(t);
				normalIndices.push_back(n);
				file >> v >> chlit('/') >> t >> chlit('/') >> n;
				vertexIndices.push_back(v);
				uvIndices.push_back(t);
				normalIndices.push_back(n);
				file >> v >> chlit('/') >> t >> chlit('/') >> n;
				vertexIndices.push_back(v);
				uvIndices.push_back(t);
				normalIndices.push_back(n);
			}
			else {
				continue;
			}
		}
		file.close();
	}
	else {
		std::cout << "Unable to open file";
		return false;
	}
	
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vindex = vertexIndices[i];
		vmath::vec4 vertex = temp_vertices[vindex-1]; //indices are 1-indexed
		out_vertices.push_back(vertex);

		vindex = normalIndices[i];
		vmath::vec3 normal = temp_normals[vindex - 1]; //indices are 1-indexed
		out_normals.push_back(normal);

		vindex = uvIndices[i];
		vmath::vec2 tc = temp_uvs[vindex - 1]; //indices are 1-indexed
		out_uvs.push_back(tc);
	}


	return true;
}

void maze_render_app::generate_grass(int** level, int width, int height, std::vector< vmath::vec3 > & out_grass) {
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++) {
			if (level[r][c] == 0) {
				float row = convert_to_vert(r, height);
				float col = convert_to_vert(c, width);
				for (float n = 0.0f; n < grass_blades; n++) {
					for (float m = 0.0f; m < grass_blades; m++) {
						float x = (n / grass_blades) + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
						float z = (m / grass_blades) + (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
						out_grass.push_back(vmath::vec3(x + col, 0.0f, z + row));
					}
				}
			}
		}
	}
}

float maze_render_app::convert_to_vert(int coord, int dim) {
	return static_cast<float>((coord * 2) - dim) - 1.0f;
}

int maze_render_app::convert_to_coord(float pos, int dim) {
	return (floor(pos + 1.0f) + dim) / 2;
}

//This is awful
void maze_render_app::wall_collision(float & xPos, float & zPos, vmath::vec3 direction, int height, int width, int** level) {
	vmath::vec3 cr = vmath::cross(direction, vmath::vec3(0.0, 1.0, 0.0));
	vmath::vec3 curr = vmath::vec3(xPos, 0.0f, zPos);
	float scalar = 0.30f;
	std::vector< vmath::vec3 > dirs;
	dirs.push_back(curr + (direction*scalar));
	dirs.push_back(curr - (direction*scalar));
	dirs.push_back(curr + (cr*scalar));
	dirs.push_back(curr - (cr*scalar));
	//cast into 4 directions, make sure none collide
	for (int i = 0; i < 4; i++) {
		vmath::vec3 point = dirs[i];
		float x = point[0];
		float z = point[2];

		int row = maze_render_app::convert_to_coord(z, height);
		int col = maze_render_app::convert_to_coord(x, width);
		if (level[row][col] == 1) {
			//collision, need to resolve
			float xdiff = x - floor(x);
			float zdiff = z - floor(z);

			float xdiff2 = xdiff - 1.0f;
			float zdiff2 = zdiff - 1.0f;

			float xsmal = (xdiff < abs(xdiff2)) ? xdiff : xdiff2;
			float zsmal = (zdiff < abs(zdiff2)) ? zdiff : zdiff2;

			if (abs(xsmal) > abs(zsmal)) {
				//eject out to nearest col
				xPos -= xsmal;
				break;
			}
			else {
				//nearest row
				zPos -= zsmal;
				break;
			}
		}
		else if (level[row][col] == 3) {
			//you win
		}
	}
}

DECLARE_MAIN(maze_render_app)