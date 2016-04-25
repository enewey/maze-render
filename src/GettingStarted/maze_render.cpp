#include <sb7.h>
#include <vmath.h>
#include <object.h>
#include <sb7ktx.h>
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
		info.windowWidth = 512;
		info.windowHeight = 512;
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
	int ** load_level(std::string filename, int & width, int & height);
	//Convert an array index into a vertex in the maze
	float convert_to_vert(int coord, int dim);
	int convert_to_coord(float pos, int dim);

	void wall_collision(float & xPos, float & zPos, vmath::vec3 direction, int height, int width, int** level);

	//Programs
	GLuint			walls_program;
	GLuint			grass_program;
	
	//Uniforms
	struct uniforms_block
	{
		vmath::mat4     mv_matrix;
		vmath::mat4     view_matrix;
		vmath::mat4     proj_matrix;
		vmath::vec4		ball_color;
	};
	GLuint          uniforms_buffer;

	// Variables for mouse interaction
	bool bPerVertex;
	bool bShiftPressed = false;
	bool bZoom = false;
	bool bRotate = false;
	bool bPan = false;

	int iWidth = info.windowWidth;
	int iHeight = info.windowHeight;

	//Sphere generation members
	/*int stacks = 10;
	int slices = 12;
	float radius = 1.0f;
	GLint sphere_triangles;
	GLfloat* sphere_data;*/

	// Rotation and Translation matricies for moving the camera by mouse interaction.
	vmath::mat4 rotationMatrix = vmath::mat4::identity();
	vmath::mat4 translationMatrix = vmath::mat4::identity();

	//vmath::mat4 directionMatrix = vmath::mat4::identity();

private:
	// Variables for mouse position to solve the arcball vectors
	int iPrevMouseX = 0;
	int iPrevMouseY = 0;
	int iCurMouseX = 0;
	int iCurMouseY = 0;

	// Scale of the objects in the scene
	float fScale = 4.0f;
	float roomScale = 20.0f;

	// Initial position of the camera
	float fXpos = 0.0f;
	float fYpos = 0.0f;
	float fZpos = 75.0f;

	bool * dirPress;
	float lookingAngle = 0.0f;
	vmath::vec3 direction;
	vmath::vec3 position;
	float cXpos = 1.0f;
	float cYpos = 0.0f;
	float cZpos = -3.0f;

	float lightY = 1.5f;

	double timeElapsed = 0.0;

	int _width, _height;
	int ** _level;
	
	GLuint floor_buffer;

	//Vertex array objects
	GLuint vao2;
	GLuint grass_vao;
	GLuint floor_vao;

	GLuint wall_tex_buffer;
	GLuint wall_normal_buffer;
	GLuint floor_normal_buffer;
	GLuint tex_toon;
	GLuint grass_tex;

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

	//Each floor will have n^2 blades of grass
	int grass_blades = 10;

	sb7::object object;
};

void maze_render_app::startup()
{
	dirPress = new bool[6];//w, d, s, a, right, left
	for (int i = 0; i < 6; i++)
		dirPress[i] = false;

	direction = vmath::vec3(0.0f, 0.0f, -1.0f);
	//position = vmath::vec3(0.0f, 3.0f, 0.0f);

	// Create program for the spinning cube
	//color_fragment_cube_program = glCreateProgram();
	walls_program = glCreateProgram();
	grass_program = glCreateProgram();
	GLint success = 0;

	//Load per-fragment phong shaders
	GLuint pvfs;
	GLuint pvvs;

	//Load per-vertex phong shaders
	pvvs = sb7::shader::load("walls-vertex.glsl", GL_VERTEX_SHADER);
	pvfs = sb7::shader::load("walls-fragment.glsl", GL_FRAGMENT_SHADER);
	glAttachShader(walls_program, pvvs);
	glAttachShader(walls_program, pvfs);
	glLinkProgram(walls_program);
	success = 0;
	glGetProgramiv(walls_program, GL_LINK_STATUS, &success);
	assert(success != GL_FALSE);

	pvvs = sb7::shader::load("grass-vertex.glsl", GL_VERTEX_SHADER);
	pvfs = sb7::shader::load("grass-fragment.glsl", GL_FRAGMENT_SHADER);
	glAttachShader(grass_program, pvvs);
	glAttachShader(grass_program, pvfs);
	glLinkProgram(grass_program);
	success = 0;
	glGetProgramiv(grass_program, GL_LINK_STATUS, &success);
	assert(success != GL_FALSE);

	glUseProgram(walls_program);
	//fragment_program = false;

	//Load textures
	load_image("bin\\media\\textures\\wall.png", &wall_tex_buffer);
	load_image("bin\\media\\textures\\normal.png", &wall_normal_buffer);
	load_image("bin\\media\\textures\\floor_normal.png", &floor_normal_buffer);	
	load_image("bin\\media\\textures\\grass_tex.png", &grass_tex);
	//Object data loaded from file
	bool res = load_object("bin\\media\\objects\\wall_data.obj", vertices, uvs, normals);
	assert(res);
	res = load_object("bin\\media\\objects\\floor_data.obj", fvertices, fuvs, fnormals);
	assert(res);

	_level = load_level("bin\\media\\objects\\walls.mdf", _width, _height);

	generate_grass(_level, _width, _height, grass_points);
	
#pragma region Wall buffers

	//Cubes vao
	glGenVertexArrays(1, &vao2);
	glBindVertexArray(vao2);

	//Texture coords buffer
	glGenBuffers(1, &tc_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, tc_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		uvs.size() * sizeof(vmath::vec2),
		&uvs[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Cube buffer
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(vmath::vec4),
		&vertices[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Normals for cube buffer
	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		normals.size() * sizeof(vmath::vec3),
		&normals[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
#pragma endregion

#pragma region Floor Buffers
	//Floor vao
	glGenVertexArrays(1, &floor_vao);
	glBindVertexArray(floor_vao);

	//Texture coords buffer
	glGenBuffers(1, &ftc_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, ftc_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		fuvs.size() * sizeof(vmath::vec2),
		&fuvs[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Vertex buffer
	glGenBuffers(1, &fbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, fbuffer);
	glBufferData(GL_ARRAY_BUFFER,
		fvertices.size() * sizeof(vmath::vec4),
		&fvertices[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Normals buffer
	glGenBuffers(1, &fnormal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, fnormal_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		fnormals.size() * sizeof(vmath::vec3),
		&fnormals[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#pragma endregion

#pragma region Grass buffers
	glGenVertexArrays(1, &grass_vao);
	glBindVertexArray(grass_vao);

	glGenBuffers(1, &grass_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, grass_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		grass_points.size() * sizeof(vmath::vec3),
		&grass_points[0],
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	
	glViewport(0, 0, info.windowWidth, info.windowHeight);
	glClearBufferfv(GL_COLOR, 0, zeros);
	glClearBufferfv(GL_DEPTH, 0, ones);


#pragma region Camera positioning with key detection
	vmath::vec3 view_position = vmath::vec3(cXpos, 0.0f, cZpos);
	if (f - timeElapsed > 0.01) {
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

	// Set up view and perspective matrix
	vmath::mat4 view_matrix = vmath::lookat(view_position,
		view_position + direction,
		vmath::vec3(0.0f, 1.0f, 0.0f));
	view_matrix *= translationMatrix;

	vmath::mat4 perspective_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);

	uniforms_block* block;
	vmath::mat4 model_matrix;

	//Cube
	glUseProgram(walls_program);

	GLint tex_brick_location = glGetUniformLocation(walls_program, "tex");
	GLint bump_tex_location = glGetUniformLocation(walls_program, "bump_map");

	glUniform1i(tex_brick_location, 0);
	glUniform1i(bump_tex_location, 1);

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

	//Draw room
	GLint light_loc = glGetUniformLocation(walls_program, "light_pos");
	glUniform4f(light_loc, cXpos, lightY, cZpos, 1.0f);
	//glUniform4f(light_loc, 0.0f, 3.0f, 0.0f, 1.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

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
	
#pragma region Floor Rendering
	//Begin Floor
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

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	//model_matrix =
	//	vmath::scale(1.0f);

	block->mv_matrix = view_matrix;// *model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	//change_settings(1, 1, 0, 0);
	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, fvertices.size());
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion

#pragma region Grass rendering
	glUseProgram(grass_program);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1i(glGetUniformLocation(grass_program, "grass"), 0);
	glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 2
	glBindTexture(GL_TEXTURE_2D, grass_tex);

	light_loc = glGetUniformLocation(grass_program, "light_pos");
	glUniform4f(light_loc, cXpos, lightY, cZpos, 1.0f);

	glBindVertexArray(grass_vao);
	glBindBuffer(GL_ARRAY_BUFFER, grass_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glVertexAttribDivisor(0, 1);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);

	//model_matrix =
	//	vmath::scale(1.0f);

	block->mv_matrix = view_matrix;// * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glDrawArrays(GL_TRIANGLES, 0, grass_points.size());
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
#pragma endregion
	////Start Cube
	//glUniform1i(invert_loc, -1); //invert the normals so cube gets proper lighting
	//
	//glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	//block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);
	//
	//model_matrix =
	//	vmath::translate(vmath::vec3(-6.0f, -16.0f, -6.0f)) *
	//	vmath::rotate(45.0f, vmath::vec3(0.0f, 1.0f, 0.0f)) *
	//	vmath::scale(fScale);
	//
	//block->mv_matrix = view_matrix * model_matrix;
	//block->view_matrix = view_matrix;
	//block->proj_matrix = perspective_matrix;
	//block->ball_color = vmath::vec4(0.1f, 0.1f, 1.0f, 1.0f);
	//
	////change_settings(0, 0, use_pos_color, 0);
	//
	//glCullFace(GL_BACK);
	//glDrawArrays(GL_TRIANGLES, 0, 36);
	//glUnmapBuffer(GL_UNIFORM_BUFFER);
	//
	////End cube
	//
	//Floor
	/*glUseProgram(floor_program);
	glBindVertexArray(floor_vao);
	//
	GLint floor_bump_location = glGetUniformLocation(floor_program, "bump_map");
	glUniform1i(floor_bump_location, 2);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, floor_normal_buffer);
	//
	//
	glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	//
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);
	//
	model_matrix =
		vmath::translate(0.0f, -roomScale + 0.01f, 0.0f) *
		vmath::scale(roomScale);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;
	block->ball_color = vmath::vec4(0.3f, 0.3f, 0.3f, 1.0f);

	//change_settings(1, 1, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	*/
	//End Floor
	//
	//Sphere
	//if (fragment_program) {
	//	glUseProgram(color_fragment_cube_program);
	//}
	//else {
	//	glUseProgram(toonshading_program);
	//}
	//
	//glBindVertexArray(object.get_vao());
	//
	//glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	//block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT);
	//
	//model_matrix =
	//	vmath::translate(vmath::vec3(-6.0f, -4.0f, -6.0f)) *
	//	vmath::scale(fScale * 2);
	//
	//block->mv_matrix = view_matrix * model_matrix;
	//block->view_matrix = view_matrix;
	//block->proj_matrix = perspective_matrix;
	//block->ball_color = vmath::vec4(1.0f, 0.1f, 0.5f, 1.0f);
	//
	////change_settings(1, 1, use_pos_color, 1);
	//
	//glCullFace(GL_BACK);
	//object.render(); //render call for sphere
	//glUnmapBuffer(GL_UNIFORM_BUFFER);
	//End Sphere

}

void maze_render_app::onKey(int key, int action)
{
	// Check to see if shift was pressed
	if (action == GLFW_PRESS/* && (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)*/)
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
	if (action)
	{
		switch (key)
		{
		//case 'R': //reset camera to original position
		//	rotationMatrix = vmath::mat4::identity();
		//	translationMatrix = vmath::mat4::identity();
		//	fXpos = 0.0f;
		//	fYpos = 0.0f;
		//	fZpos = 75.0f;
		//	break;

		//case 'V': //switch between phong and toonshading
		//	fragment_program = !fragment_program;
		//	break;
		//default:
		//	break;
		}
	}
	// Check to see if shift was released
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
	int x, y;

	//getMousePosition(x, y);
	//// Check to see if left mouse button was pressed for rotation
	//if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
	//	bRotate = true;
	//	iPrevMouseX = iCurMouseX = x;
	//	iPrevMouseY = iCurMouseY = y;
	//}
	//// Check to see if right mouse button was pressed for zoom and pan
	//else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
	//	bZoom = false;
	//	bPan = false;
	//	if (bShiftPressed == true) {
	//		bZoom = true;
	//	}
	//	else if (bShiftPressed == false) {
	//		bPan = true;
	//	}
	//	iPrevMouseX = iCurMouseX = x;
	//	iPrevMouseY = iCurMouseY = y;
	//}
	//else {
	//	bRotate = false;
	//	bZoom = false;
	//	bPan = false;
	//}

}

void maze_render_app::onMouseMove(int x, int y)
{
	// If rotating, zooming, or panning save mouse x and y
	if (bRotate || bZoom || bPan)
	{
		iCurMouseX = x;
		iCurMouseY = y;
	}
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
	//std::string filename = "bin\\media\\textures\\wall.png";
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
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, iwidth, iheight);
	

	//genCheckerboard(checkers); //generates a checkerboard with X checkers
	glTexSubImage2D(GL_TEXTURE_2D,  // 2D texture
		0,              // Level 0
		0, 0,           // Offset 0, 0
		iwidth, iheight,       // 256 x 256 texels, replace entire image
		GL_RGBA,        // Four channel data
		GL_UNSIGNED_BYTE,       // data type
		&tex[0]);
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

int ** maze_render_app::load_level(std::string filename, int & width, int & height) {
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

		while (file >> r >> c) {
			level[c][r] = 1;
		}
	}

	return level;
}

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
	/*int flor = floor(pos);
	int ceil = floor(pos+1.0f);
	if (flor % 2 == 0) pos = flor;
	else if (ceil % 2 == 0) pos = ceil;*/

	return (floor(pos + 1.0f) + dim) / 2;
}

void maze_render_app::wall_collision(float & xPos, float & zPos, vmath::vec3 direction, int height, int width, int** level) {
	vmath::vec3 cr = vmath::cross(direction, vmath::vec3(0.0, 1.0, 0.0));
	vmath::vec3 curr = vmath::vec3(xPos, 0.0f, zPos);
	float scalar = 0.33f;
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
	}

	//for now, this block constitutes one collision check (center position)
	// i want to check if +0.1 in front, back, left, and right of player are also not colliding
	
}

DECLARE_MAIN(maze_render_app)