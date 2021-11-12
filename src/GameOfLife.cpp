#include "iostream"
#include "string"
#include "fstream"
#include "sstream"

#include <memory>

#include"GL/glew.h"
#include "GLFW/glfw3.h"
#include <bitset>

#include "../include/shader-printf/shaderprintf.h"

const int RAND_BOOL = RAND_MAX / 2;
const int WORK_GROUP_SIZE = 32;
const int DATA_W = 512;
const int DATA_H = 512;

const int COMPUTE_GROUP_X = DATA_W / WORK_GROUP_SIZE;
const int COMPUTE_GROUP_Y = DATA_H / WORK_GROUP_SIZE;
const double FPS_LIMIT = 1.0 / 20.0;

const float zoomFactorIn = 1.0f / 50.0f;
const float zoomFactorOut = 1.0f / 50.0f;

bool _playSim = true;
float cursor_x = 0;
float cursor_y = 0;

float point_x = 0;
float point_y = 0;
float zoom_x = 0;
float zoom_y = 0;
float window_x = 0;
float window_y = 0;
float zoom_scalar = 1;
float mapOffset_x = 0;
float mapOffset_y = 0;
float mapOffset_z = 0;
float mapOffset_w = 0;

int* data = new int[DATA_W * DATA_H];
float* verticies;

GLuint dataSSbo = (unsigned int)NULL;
GLuint nextSSbo = (unsigned int)NULL;
GLuint VBO = (unsigned int)NULL;
GLuint VAO = (unsigned int)NULL;
GLuint shader = (unsigned int)NULL;
GLuint compute = (unsigned int)NULL;
GLFWwindow* window = NULL;

void consoleOutStatus(const std::string status) {
	std::cout << status << std::endl;
}

bool randomBool() {
   return rand() > RAND_BOOL;
}

void SetWindowHints() {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
}

void HandleKeyInput(GLFWwindow* window, int key, int status, int action, int mods);

std::string ReadFile(const std::string& path){
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

GLuint CreateAndCompileShader(const std::string& path, GLuint shaderType) {
	GLuint shader = glCreateShader(shaderType);

	const std::string shaderSource = ReadFile(path);
	const char* cShader = shaderSource.c_str();
	glShaderSource(shader, 1, &cShader, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader error: " << infoLog << std::endl;
	}

	return shader;
}

GLuint CreateAndLinkProgram() {
	//GLuint vertexShader = CreateAndCompileShader("../../../../shaders/vert.vert", GL_VERTEX_SHADER);
	//GLuint fragmentShader = CreateAndCompileShader("../../../../shaders/frag.frag", GL_FRAGMENT_SHADER);

	GLuint vertexShader = CreateAndCompileShader("../../shaders/vert.vert", GL_VERTEX_SHADER);
	GLuint fragmentShader = CreateAndCompileShader("../../shaders/frag.frag", GL_FRAGMENT_SHADER);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader error: " << infoLog << std::endl;
	}

	return program;

}

GLuint CreateAndLinkComputeShader(){
	GLuint computeShader = CreateAndCompileShader("../../shaders/compute.comp", GL_COMPUTE_SHADER);
	GLuint program = glCreateProgram();
	glAttachShader(program, computeShader);

	glLinkProgram(program);

	int success = 1;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader error: " << infoLog << std::endl;
	}

	return program;	
}

float* GenerateVertecies();

void SetCell(int *data, int x, int y);

void cleanup();

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	float oldScalar = zoom_scalar;

	if(yoffset > 0 && zoom_scalar - zoomFactorIn > 0){
		//scroll up
		zoom_scalar -= zoomFactorIn;

	}else if (yoffset < 0 && zoom_scalar + zoomFactorOut <= 2){
		//scroll down
		zoom_scalar += zoomFactorOut;
	}

	float dif_scalar = zoom_scalar - oldScalar;

	point_x -= -(cursor_x * dif_scalar);
	point_y -= -(cursor_y * dif_scalar);

	mapOffset_x = (point_x/window_x)*DATA_W;
	mapOffset_y = (((window_x*zoom_scalar)-point_x)/window_x)*DATA_W;
	mapOffset_z = (point_y/window_y)*DATA_H;
	mapOffset_w = (((window_y*zoom_scalar)-point_y)/window_y)*DATA_H;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	cursor_x = (float)xpos;
	cursor_y = (float)ypos;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height){
	glViewport(0, 0, width, height);
	if(zoom_x == window_x){
		zoom_x = (float)width;
		zoom_y = (float)height;
	}

	window_x = (float)width;
	window_y = (float)height;
}

int main()
{
	atexit(cleanup);
	at_quick_exit(cleanup);

	consoleOutStatus("---- FAST GAMEOFLIFE ----");
	consoleOutStatus("--- Initializing GLFW ---");

	if (!glfwInit()) {
		std::cerr << "glfw initialization failed" << std::endl;
		return -1;
	}


	consoleOutStatus("--- Creating Window ---");

	SetWindowHints();

	window = glfwCreateWindow(800, 400, "Fast GameOfLife", nullptr, nullptr);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	window_x = (float)width;
	window_y = (float)height;
	zoom_x = (float)width;
	zoom_y = (float)height;

	mapOffset_y = (float)DATA_W;
	mapOffset_w = (float)DATA_H;

	if (window == nullptr) {
		std::cerr << "GLFW Window creation failed" << std::endl;
		glfwTerminate();
		return  -2;
	}

	consoleOutStatus("--- Setting Window Context ---");

	glfwMakeContextCurrent(window);

	consoleOutStatus("--- Initializing GLEW & Keyboard Callbacks ---");

	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW Init failed" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -3;
	}

	glfwSetKeyCallback(window, HandleKeyInput);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

	consoleOutStatus("--- Create & Compile Shaders ---");

	shader = CreateAndLinkProgram();
	compute = CreateAndLinkComputeShader();

	consoleOutStatus("--- Generate Verticies and Buffers ---");

	verticies = GenerateVertecies();


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO); 
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, verticies, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	consoleOutStatus("--- Initialize Data Matrix ---");

	//Init GameOfLife matricies

	//memset(data, 0, DATA_W * DATA_H*sizeof(int));

	srand (time(NULL));
	for (int i = 0; i < DATA_W; i++)
	{
		for (int j = 0; j < DATA_H; j++)
		{
			if (randomBool())
				SetCell(data, i, j);
		}
	}

	consoleOutStatus("--- Initialize Data SSBos---");


	//Init GameOfLife Shader Storage Buffer Objects


	glGenBuffers(1, &dataSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), data, GL_STATIC_DRAW);



	glGenBuffers(1, &nextSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), NULL, GL_STATIC_DRAW);

	int uniform_WindowSize = glGetUniformLocation(shader,"WindowSize");
	int uniform_MapOffset = glGetUniformLocation(shader,"MapOffset");

	consoleOutStatus("--- Start Game Loop---");


	//Game loop

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	bool round = false;
	double lastUpdateTime = 0.0;
	double lastFrameTime = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		double now = glfwGetTime();
		double delta = now - lastUpdateTime;
		if ((now - lastFrameTime) >= FPS_LIMIT) {
			//Clear colors
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//compute generation
			if (!round) {
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, dataSSbo);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, nextSSbo);
			}
			else {
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, nextSSbo);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, dataSSbo);
			}


			if (_playSim) {
				glUseProgram(compute);
				glDispatchCompute(COMPUTE_GROUP_X, COMPUTE_GROUP_Y, 1);

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			glViewport(0, 0, width, height);

			glBindVertexArray(VAO);
			glUseProgram(shader);
			glUniform2f(uniform_WindowSize, width, height);
			glUniform4f(uniform_MapOffset, mapOffset_x, mapOffset_y, mapOffset_z, mapOffset_w);

			glDrawArrays(GL_TRIANGLES, 0, 12);

			if (_playSim)
				round = !round;

			glfwSwapBuffers(window);

			lastFrameTime = now;
		}
		lastUpdateTime = now;
		
	}

	return 0;
}

void cleanup() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	if(VAO != NULL)
		glDeleteVertexArrays(1, &VAO);

	if (VBO != NULL)
		glDeleteVertexArrays(1, &VBO);

	if (dataSSbo != NULL)
		glDeleteVertexArrays(1, &dataSSbo);

	if (nextSSbo != NULL)
		glDeleteVertexArrays(1, &nextSSbo);

	if (shader != NULL)
		glDeleteProgram(shader);

	if (compute != NULL)
		glDeleteProgram(compute);


	glfwDestroyWindow(window);
	glfwTerminate();

	delete[] data;
	delete[] verticies;

}

void HandleKeyInput(GLFWwindow* window, int key, int status, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		//play sim.
		_playSim = !_playSim;
	}else if(key == GLFW_KEY_B && action == GLFW_PRESS){
		point_x = 0;
		point_y = 0;
		zoom_scalar = 1;
		zoom_x = window_x;
		zoom_y = window_y;
		mapOffset_x = (float)0;
		mapOffset_y = (float)DATA_W;
		mapOffset_z = (float)0;
		mapOffset_w = (float)DATA_H;
	}
}

float* GenerateVertecies() {
	float* vertecies = new float[12]{
		-1.0f,1.0f,-1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f
	};

	return vertecies;
}

void SetCell(int* data, int x, int y) {
	data[x+y*DATA_W] |= 1;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if (i != 0 ||j != 0) {
				data[((x + i + DATA_W) % DATA_W) + ((y + j + DATA_H) % DATA_H) * DATA_W] += 2;
			}
		}
	}
}
