#include "iostream"
#include "string"
#include "fstream"
#include "sstream"

#include"GL/glew.h"
#include "GLFW/glfw3.h"

clock_t start, cycles, min = LONG_MAX, max = 0, avg, current;
long runs = 0;

const int RAND_BOOL = RAND_MAX / 2;
const int WORK_GROUP_SIZE = 20;
const int DATA_W = 2500;
const int DATA_H = 2500;

bool _playSim = true ;

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
	//glGetProgramiv(program, GL_LINK_STATUS, &success);

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

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "Shader error: " << infoLog << std::endl;
	}

	return program;	
}

float* GenerateVerteciesOld(int data_w, int data_h) {
	float* vertecies = new float[24*data_w*data_h];

	float factor_w = (float)2 / (float)data_w;
	float factor_h = (float)2 / (float)data_h;

	for (int i = 0; i < data_w; i++)
	{
		for (int j = 0; j < data_h; j++)
		{
			vertecies[24 * (i + (j * data_w))] = -1 + factor_w * i;
			vertecies[24 * (i + (j * data_w)) + 1] = 1-factor_h*j;
			vertecies[24 * (i + (j * data_w)) + 2] = i;
			vertecies[24 * (i + (j * data_w)) + 3] = j;

			vertecies[24 * (i + (j * data_w)) + 4] = -1 + factor_w * i;
			vertecies[24 * (i + (j * data_w)) + 5] = 1-factor_h*(j+1);
			vertecies[24 * (i + (j * data_w)) + 6] = i;
			vertecies[24 * (i + (j * data_w)) + 7] = j;

								
			vertecies[24 * (i + (j * data_w)) + 8] = -1 + factor_w *(i+1);
			vertecies[24 * (i + (j * data_w)) + 9] = 1-factor_h*(j);
			vertecies[24 * (i + (j * data_w)) + 10] = i;
			vertecies[24 * (i + (j * data_w)) + 11] = j;
									
			vertecies[24 * (i + (j * data_w)) + 12] = -1+factor_w*(i);
			vertecies[24 * (i + (j * data_w)) + 13] = 1-factor_h*(j+1);
			vertecies[24 * (i + (j * data_w)) + 14] = i;
			vertecies[24 * (i + (j * data_w)) + 15] = j;
									
			vertecies[24 * (i + (j * data_w)) + 16] = -1+factor_w*(i+1);
			vertecies[24 * (i + (j * data_w)) + 17] = 1-factor_h*(j+1);
			vertecies[24 * (i + (j * data_w)) + 18] = i;
			vertecies[24 * (i + (j * data_w)) + 19] = j;
				
			vertecies[24 * (i + (j * data_w)) + 20] = -1+factor_w*(i+1);
			vertecies[24 * (i + (j * data_w)) + 21] = 1-factor_h*(j);
			vertecies[24 * (i + (j * data_w)) + 22] = i;
			vertecies[24 * (i + (j * data_w)) + 23] = j;

		}
	}

	return vertecies;
}

float* GenerateVertecies(int data_w, int data_h) {
	float* vertecies = new float[12]{
		-1.0f,1.0f,-1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f
	};

	return vertecies;
}

int main()
{
	if (!glfwInit()) {
		std::cerr << "glfw initialization failed" << std::endl;
		return -1;
	}

	float* verticies = GenerateVertecies(DATA_W,DATA_H);

	SetWindowHints();

	GLFWwindow* window = glfwCreateWindow(800, 400, "Fast GameOfLife", nullptr, nullptr);

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

	srand (time(NULL));

	auto data = new int[DATA_W][DATA_H];
	for (int i = 0; i < DATA_W; i++)
	{
		for (int j = 0; j < DATA_H; j++)
		{
			data[i][j] = randomBool() ? 1 : 0;
		}
	}

	auto next = new int[DATA_W][DATA_H];

	//Init GameOfLife Shader Storage Buffer Objects

	GLuint dataSSbo;
	glGenBuffers(1, &dataSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, dataSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), data, GL_STATIC_DRAW);


	GLuint nextSSbo;
	glGenBuffers(1, &nextSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, DATA_W * DATA_H * sizeof(int), next, GL_STATIC_DRAW);

	int uniform_WindowSize = glGetUniformLocation(shader,"WindowSize");



	//Game loop

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	bool round = false;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		//Clear colors
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

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
			glDispatchCompute(DATA_W / WORK_GROUP_SIZE, DATA_H / WORK_GROUP_SIZE, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
		


		glBindVertexArray(VAO);
		glUseProgram(shader);
		glUniform2f(uniform_WindowSize, width, height);


		glDrawArrays(GL_TRIANGLES, 0, 12);

		if(_playSim)
			round = !round;

		glfwSwapBuffers(window);
		


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
	delete[] next;
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