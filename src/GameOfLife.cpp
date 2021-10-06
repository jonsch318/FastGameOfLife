#include "iostream"
#include "string"
#include "fstream"
#include "sstream"

#include <memory>

#include"GL/glew.h"
#include "GLFW/glfw3.h"
#include <bitset>

const int RAND_BOOL = RAND_MAX / 2;
const int WORK_GROUP_SIZE = 32;
const int DATA_W = 8192;
const int DATA_H = 8192;

const int COMPUTE_GROUP_X = DATA_W / WORK_GROUP_SIZE;
const int COMPUTE_GROUP_Y = DATA_H / WORK_GROUP_SIZE;
const double FPS_LIMIT = 1.0 / 20.0;

bool _playSim = true;

auto data = new int[DATA_W * DATA_H];


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

	int success = 1;
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

int main()
{
	if (!glfwInit()) {
		std::cerr << "glfw initialization failed" << std::endl;
		return -1;
	}

	float* verticies = GenerateVertecies();

	SetWindowHints();

	GLFWwindow* window = glfwCreateWindow(800, 400, "Fast GameOfLife", nullptr, nullptr);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	if (window == nullptr) {
		std::cerr << "GLFW Window creation failed" << std::endl;
		glfwTerminate();
		return  -2;
	}

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, HandleKeyInput);
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW Init failed" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -3;
	}


	GLuint shader = CreateAndLinkProgram();
	GLuint compute = CreateAndLinkComputeShader();

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO); 
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, verticies, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	//Init GameOfLife matricies

	memset(data, 0, DATA_W * DATA_H*sizeof(int));

	srand (time(NULL));
	for (int i = 0; i < DATA_W; i++)
	{
		for (int j = 0; j < DATA_H; j++)
		{
			if (randomBool())
				SetCell(data, i, j);
		}
	}

	//Init GameOfLife Shader Storage Buffer Objects

	GLuint dataSSbo;
	glGenBuffers(1, &dataSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), data, GL_STATIC_DRAW);


	GLuint nextSSbo;
	glGenBuffers(1, &nextSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), NULL, GL_STATIC_DRAW);

	int uniform_WindowSize = glGetUniformLocation(shader,"WindowSize");



	//Game loop

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

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

			glDrawArrays(GL_TRIANGLES, 0, 12);

			if (_playSim)
				round = !round;

			glfwSwapBuffers(window);

			lastFrameTime = now;
		}
		lastUpdateTime = now;
		
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &dataSSbo);
	glDeleteBuffers(1, &nextSSbo);
	glDeleteProgram(shader);

	glfwDestroyWindow(window);
	glfwTerminate();

	//Cleanup

	delete[] data;
	//delete[] next;
	delete[] verticies;
	return 0;
}

void HandleKeyInput(GLFWwindow* window, int key, int status, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		//play sim.
		_playSim = !_playSim;
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
