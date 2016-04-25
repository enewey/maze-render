#include <sb6.h>
#include <vmath.h>

#include <object.h>
#include <sb6ktx.h>
#include <shader.h>
#include <assert.h>
#include <cmath>

#define PI 3.14159265

class gettingStarted_app : public sb6::application
{
public:
    gettingStarted_app()
        : per_fragment_program(0),
          per_vertex_program(0),
          bPerVertex(false)
    {
    }

protected:
    void init()
    {
        static const char title[] = "gettingStarted";

        sb6::application::init();

        memcpy(info.title, title, sizeof(title));
		info.windowWidth = 512;
		info.windowHeight = 512;
    }

    void startup();
    void render(double currentTime);
    void onKey(int key, int action);
	void onMouseMove(int x, int y);
	void onMouseButton(int button, int action);
	vmath::vec3 getArcballVector(int x, int y);

    void loadShaders();

    GLuint          per_fragment_program;
    GLuint          per_vertex_program;

    struct
    {
        GLuint      color;
        GLuint      normals;
    } textures;

    struct uniforms_block
    {
        vmath::mat4     mv_matrix;
        vmath::mat4     view_matrix;
        vmath::mat4     proj_matrix;
    };

    GLuint          uniforms_buffer;

    struct
    {
		GLint			oneColor;
        GLint           diffuse_albedo;
        GLint           specular_albedo;
        GLint           specular_power;
		GLint			light_pos;
    } uniforms[2];

    sb6::object     sphere;
	sb6::object     cube;

	// Variables for mouse interaction
    bool bPerVertex;
	bool bShiftPressed = false;
	bool bZoom = false;
	bool bRotate = false;
	bool bPan = false;

	int iWidth = info.windowWidth;
	int iHeight = info.windowHeight;

	// Variable for testing if I can pass variables to the vertex shader.
	// If the user presses 'C' the color changes to a solid color on the
	// sphere.
	int oneColor = 0;

	// Rotation and Translation matricies for moving the camera by mouse interaction.
	vmath::mat4 rotationMatrix = vmath::mat4::identity();
	vmath::mat4 translationMatrix = vmath::mat4::identity();

private:
	// Variables for mouse position to solve the arcball vectors
	int iPrevMouseX = 0;
	int iPrevMouseY = 0;
	int iCurMouseX = 0;
	int iCurMouseY = 0;

	// Scale of the objects in the scene
	float fScale = 7.0f;

	// Initial position of the camera
	float fXpos = 0.0f;
	float fYpos = 0.0f;
	float fZpos = 75.0f;

	GLuint buffer;
	GLuint vao2;
};

void gettingStarted_app::startup()
{

	loadShaders(); 
	
	// Buffer for room and sphere objects
	glGenBuffers(1, &uniforms_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms_block), NULL, GL_DYNAMIC_DRAW);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);	
}

void gettingStarted_app::render(double currentTime)
{
    static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
	static const GLfloat skyBlue[] = { 0.529f, 0.808f, 0.922f };
    static const GLfloat ones[] = { 1.0f };
    const float f = (float)currentTime;
	
	// Calculations for mouse interaction camera rotation and translation matrix
	float fAngle = 0.0f;
	vmath::vec3 axis_in_camera_coord = (0.0f, 1.0f, 0.0f);	
	if (iCurMouseX != iPrevMouseX || iCurMouseY != iPrevMouseY) {
		// Arcball Rotation
		if (bRotate){
			vmath::vec3 va = getArcballVector(iPrevMouseX, iPrevMouseY);
			vmath::vec3 vb = getArcballVector(iCurMouseX, iCurMouseY);
			fAngle = acos(fmin(1.0f, vmath::dot(va, vb)));
			axis_in_camera_coord = vmath::cross(va, vb);
			axis_in_camera_coord = vmath::normalize(axis_in_camera_coord);
			iPrevMouseX = iCurMouseX;
			iPrevMouseY = iCurMouseY;
			rotationMatrix *= vmath::rotate(vmath::degrees(fAngle), axis_in_camera_coord);
		}
		// Zoom in and out
		if (bZoom) {
			fZpos += (iCurMouseY - iPrevMouseY);
			if (fZpos > 500)
			{
				fZpos = 500;
			}
			else if (fZpos < 10)
			{
				fZpos = 10;
			}
			iPrevMouseY = iCurMouseY;
			iPrevMouseX = iCurMouseX;
		}
		// Pan camera left, right, up, and down
		if (bPan) {
			fXpos += (iCurMouseX - iPrevMouseX);
			fYpos += (iCurMouseY - iPrevMouseY);
			iPrevMouseY = iCurMouseY;
			iPrevMouseX = iCurMouseX;
			translationMatrix = vmath::translate(fXpos / (info.windowWidth / fZpos), -fYpos/(info.windowWidth / fZpos), 0.0f);
		}
	}

    glUseProgram(bPerVertex ? per_vertex_program : per_fragment_program);
    glViewport(0, 0, info.windowWidth, info.windowHeight);

	// Create sky blue background
    glClearBufferfv(GL_COLOR, 0, skyBlue);
    glClearBufferfv(GL_DEPTH, 0, ones);

	// Set up view and perspective matrix, and light position
	vmath::vec3 view_position = vmath::vec3(0.0f, 0.0f, fZpos);
	vmath::mat4 view_matrix = vmath::lookat(view_position,
		vmath::vec3(0.0f, 0.0f, 0.0f),
		vmath::vec3(0.0f, 1.0f, 0.0f));
	view_matrix *= translationMatrix;
	view_matrix *= rotationMatrix;

	vmath::mat4 perspective_matrix = vmath::perspective(50.0f, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);

    vmath::vec3 light_position = vmath::vec3(50.0f, 20.0f, 75.0f);

	// Render the room using cube object from OpenGL SuperBible
	cube.load("..\\..\\bin\\media\\objects\\cube.sbm");

	glCullFace(GL_FRONT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
	uniforms_block * block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	vmath::mat4 model_matrix = vmath::translate(0.0f, 0.0f, 0.0f) *
		vmath::scale(20.0f);

	block->mv_matrix = view_matrix * model_matrix;
	block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glUniform1i(uniforms[bPerVertex ? 1 : 0].oneColor, 1);
	glUniform1f(uniforms[bPerVertex ? 1 : 0].specular_power, 10.0f);
	glUniform3fv(uniforms[bPerVertex ? 1 : 0].specular_albedo, 1, vmath::vec3(.75f));
	glUniform3fv(uniforms[bPerVertex ? 1 : 0].light_pos, 1, light_position);

	cube.render();

	// Render the colored sphere from OpenGL SuperBible Shpere Object
	sphere.load("..\\..\\bin\\media\\objects\\sphere.sbm");

	glCullFace(GL_BACK);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
    block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	model_matrix = vmath::translate(0.0f, 1.0f, 0.0f) * vmath::scale(fScale);

    block->mv_matrix = view_matrix * model_matrix;
    block->view_matrix = view_matrix;
	block->proj_matrix = perspective_matrix;

    glUnmapBuffer(GL_UNIFORM_BUFFER);

	glUniform1i(uniforms[bPerVertex ? 1 : 0].oneColor, oneColor);
    glUniform1f(uniforms[bPerVertex ? 1 : 0].specular_power, 30.0f);
    glUniform3fv(uniforms[bPerVertex ? 1 : 0].specular_albedo, 1, vmath::vec3(1.0f));
	glUniform3fv(uniforms[bPerVertex ? 1 : 0].light_pos, 1, light_position);

	sphere.render();


	
}

void gettingStarted_app::onKey(int key, int action)
{
	// Check to see if shift was pressed
	if (action == GLFW_PRESS && (key == GLFW_KEY_LSHIFT || key == GLFW_KEY_RSHIFT))
	{
		bShiftPressed = true;
	}
	if (action)
	{
        switch (key)
        {
			case 'C':
				if (oneColor == 0)
				{
					oneColor = 1;
				}
				else
				{
					oneColor = 0;
				}
				loadShaders();
				break;
            case 'R': 
				rotationMatrix = vmath::mat4::identity();
				translationMatrix = vmath::mat4::identity();
				fXpos = 0.0f;
				fYpos = 0.0f;
				fZpos = 75.0f;
                loadShaders();
                break;
            case 'V':
                bPerVertex = !bPerVertex;
                break;
		}
    }
	// Check to see if shift was released
	if (action == GLFW_RELEASE) {
		bShiftPressed = false;
	}
}


void gettingStarted_app::onMouseButton(int button, int action)
{
	int x, y;
	
	getMousePosition(x, y);
	// Check to see if left mouse button was pressed for rotation
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		bRotate = true;
		iPrevMouseX = iCurMouseX = x;
		iPrevMouseY = iCurMouseY = y;
	}
	// Check to see if right mouse button was pressed for zoom and pan
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		bZoom = false;
		bPan = false;
		if (bShiftPressed == true)
		{
			bZoom = true;
		}
		else if (bShiftPressed == false)
		{
			bPan = true;
		}
		iPrevMouseX = iCurMouseX = x;
		iPrevMouseY = iCurMouseY = y;
	}
	else {
		bRotate = false;
		bZoom = false;
		bPan = false;
	}
	
}

void gettingStarted_app::onMouseMove(int x, int y)
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

vmath::vec3 gettingStarted_app::getArcballVector(int x, int y) {
	// find vector from origin to point on sphere
	vmath::vec3 vecP = vmath::vec3(1.0f*x / info.windowWidth * 2 - 1.0f, 1.0f*y / info.windowHeight * 2 - 1.0f, 0.0f);
	// inverse y due to difference in origin location on the screen
	vecP[1] = -vecP[1];
	float vecPsquared = vecP[0] * vecP[0] + vecP[1] * vecP[1];
	// solve for vector z component
	if (vecPsquared <= 1)
		vecP[2] = sqrt(1-vecPsquared);		
	else
		vecP = vmath::normalize(vecP);
	return vecP;
}

void gettingStarted_app::loadShaders()
{
    GLuint vs;
    GLuint fs;

	// load per-fragment shaders
	vs = sb6::shader::load("..\\..\\bin\\media\\shaders\\phonglighting\\per-fragment-phong.vs.glsl", GL_VERTEX_SHADER);
	fs = sb6::shader::load("..\\..\\bin\\media\\shaders\\phonglighting\\per-fragment-phong.fs.glsl", GL_FRAGMENT_SHADER);

    if (per_fragment_program)
        glDeleteProgram(per_fragment_program);

    per_fragment_program = glCreateProgram();
    glAttachShader(per_fragment_program, vs);
    glAttachShader(per_fragment_program, fs);
    glLinkProgram(per_fragment_program);

	uniforms[0].oneColor = glGetUniformLocation(per_fragment_program, "oneColor");
    uniforms[0].diffuse_albedo = glGetUniformLocation(per_fragment_program, "diffuse_albedo");
    uniforms[0].specular_albedo = glGetUniformLocation(per_fragment_program, "specular_albedo");
    uniforms[0].specular_power = glGetUniformLocation(per_fragment_program, "specular_power");
	uniforms[0].light_pos = glGetUniformLocation(per_fragment_program, "light_pos");

	// load per-vertex shaders
	vs = sb6::shader::load("..\\..\\bin\\media\\shaders\\phonglighting\\per-vertex-phong.vs.glsl", GL_VERTEX_SHADER);
	fs = sb6::shader::load("..\\..\\bin\\media\\shaders\\phonglighting\\per-vertex-phong.fs.glsl", GL_FRAGMENT_SHADER);

    if (per_vertex_program)
        glDeleteProgram(per_vertex_program);

    per_vertex_program = glCreateProgram();
    glAttachShader(per_vertex_program, vs);
    glAttachShader(per_vertex_program, fs);
    glLinkProgram(per_vertex_program);

	uniforms[1].oneColor = glGetUniformLocation(per_vertex_program, "oneColor");
    uniforms[1].diffuse_albedo = glGetUniformLocation(per_vertex_program, "diffuse_albedo");
    uniforms[1].specular_albedo = glGetUniformLocation(per_vertex_program, "specular_albedo");
    uniforms[1].specular_power = glGetUniformLocation(per_vertex_program, "specular_power");
	uniforms[1].light_pos = glGetUniformLocation(per_vertex_program, "light_pos");
}

DECLARE_MAIN(gettingStarted_app)
