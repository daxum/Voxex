/******************************************************************************
 * Voxex - An experiment with sparse voxel terrain
 * Copyright (C) 2019
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <array>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.hpp"
#include "ExtraMath.hpp"

constexpr float dist = 1800.0f;
constexpr float center = 0.0f;
constexpr float height = 200.0f;

namespace {
	const std::array<float, 24> boxVerts = {
		0.0f, 0.0f,  0.0f, //0
		0.0f, 0.0f, -1.0f, //1
		0.0f, 1.0f,  0.0f, //2
		0.0f, 1.0f, -1.0f, //3
		1.0f, 0.0f,  0.0f, //4
		1.0f, 0.0f, -1.0f, //5
		1.0f, 1.0f,  0.0f, //6
		1.0f, 1.0f, -1.0f, //7
	};

	const std::array<uint32_t, 36> boxIndices = {
		//Bottom, front, left
		5, 4, 0, 5, 0, 1, 0, 4, 6, 2, 0, 6, 1, 0, 2, 1, 2, 3,
		//Right, back, top
		5, 7, 6, 4, 5, 6, 5, 1, 3, 5, 3, 7, 7, 3, 6, 2, 6, 3
	};

	const std::vector<std::pair<glm::vec3, glm::quat>> frames = {
		{{-dist + center, height, -center}, {0.0, 0.0, 0.0, 0.0}},
		{{center, height, dist - center}, {0.0, 0.0, 0.0, 0.0}},
		{{dist + center, height, -center}, {0.0, 0.0, 0.0, 0.0}},
		{{center, height, -dist - center}, {0.0, 0.0, 0.0, 0.0}},
		{{-dist + center, height, -center}, {0.0, 0.0, 0.0, 0.0}},
		{{center, height, dist - center}, {0.0, 0.0, 0.0, 0.0}},
		{{dist + center, height, -center}, {0.0, 0.0, 0.0, 0.0}}
	};
}

Render::Render() :
	window(nullptr),
	prog(0),
	vao(0),
	vertexBuffer(0),
	indexBuffer(0),
	indexCount(0),
	camPos(dist, height, -dist),
	camLook(center, center, -center),
	camUp(0.0, 1.0, 0.0),
	spline(frames, 30000) {

	init();
	loadShaders();
}

Render::~Render() {
	glBindVertexArray(0);

	glDeleteProgram(prog);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &indexBuffer);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Render::render() {
	glm::mat4 view;

	if (true) {
		const std::pair<glm::vec3, glm::quat> loc = spline.getLocation((float)ExMath::getTimeMillis());
		view = glm::lookAt(loc.first, camLook, camUp);
	}
	else {
		view = glm::lookAt(camPos, camLook, camUp);
	}

	GLuint uniformLoc = glGetUniformLocation(prog, "view");
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(view));

	uniformLoc = glGetUniformLocation(prog, "projection");
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(proj));

	uniformLoc = glGetUniformLocation(prog, "lightDir");
	glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0, -1.0, 1.0));
	sunDir = glm::vec3(view * glm::vec4(sunDir, 0.0));
	glUniform3fv(uniformLoc, 1, glm::value_ptr(sunDir));

	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*) 0);
}

void Render::present() {
	glfwSwapBuffers(window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Render::addChunkData(const std::pair<std::vector<ChunkVertex>, std::vector<uint32_t>>& chunkData) {
	size_t indexOffset = renderBoxes.size() / 9;

	for (const ChunkVertex& vert : chunkData.first) {
		renderBoxes.push_back(vert.position.x);
		renderBoxes.push_back(vert.position.y);
		renderBoxes.push_back(vert.position.z);
		renderBoxes.push_back(vert.normal.x);
		renderBoxes.push_back(vert.normal.y);
		renderBoxes.push_back(vert.normal.z);
		renderBoxes.push_back(vert.color.at(0));
		renderBoxes.push_back(vert.color.at(1));
		renderBoxes.push_back(vert.color.at(2));
	}

	for (uint32_t index : chunkData.second) {
		renderIndices.push_back(index + indexOffset);
	}
}

void Render::init() {
	if (!glfwInit()) {
		throw std::runtime_error("Couldn't initialize glfw");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(960, 540, "Chunk Render Test", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);

	if (window == nullptr) {
		throw std::runtime_error("Failed to create window and context");
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, setViewport);

	int windowWidth = 0;
	int windowHeight = 0;

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	setViewport(window, windowWidth, windowHeight);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0, 0.2, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Render::loadShaders() {
	GLuint vertexShader = createShader("shaders/basicShader.vert", GL_VERTEX_SHADER);
	GLuint fragmentShader = createShader("shaders/basicShader.frag", GL_FRAGMENT_SHADER);

	GLuint shaderProgram = glCreateProgram();

	if (shaderProgram == 0) {
		throw std::runtime_error("Program loading failed");
	}

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glLinkProgram(shaderProgram);

	int linked = 0;

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);

	if (linked == 0) {
		char infoLog[1024];
		glGetProgramInfoLog(shaderProgram, 1024, nullptr, infoLog);
		std::cout << "Program linking failed\n------------ Program Link Log ------------\n" << infoLog << "\n---------------- End Log -----------------\n";

		throw std::runtime_error("Linking failed for program!");
	}

	glDetachShader(shaderProgram, vertexShader);
	glDetachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	prog = shaderProgram;
	glUseProgram(prog);
}

void Render::initBuffers() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &indexBuffer);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, renderBoxes.size() * sizeof(float), renderBoxes.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderIndices.size() * sizeof(uint32_t), renderIndices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

	indexCount = renderIndices.size();

	std::cout << "Buffers using " << ((renderBoxes.size() * sizeof(float) + renderIndices.size() * sizeof(uint32_t)) / 1024) << "KB!\n";

	std::vector<float>().swap(renderBoxes);
	std::vector<uint32_t>().swap(renderIndices);
}

GLuint Render::createShader(std::string filename, GLenum type) {
	GLuint shader = glCreateShader(type);

	if (shader == 0) {
		throw std::runtime_error("Could not allocate shader");
	}

	std::string sourceStr = loadShaderSource(filename);
	const char* source = sourceStr.c_str();

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == 0) {
		char infoLog[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, infoLog);

		std::cout << "Failed to compile shader \"" << filename << "\"\n--------- Shader Compilation Log ---------\n" << infoLog << "\n---------------- End Log -----------------\n";

		throw std::runtime_error("Failed to compile shader \"" + filename + "\"");
	}

	return shader;
}

std::string Render::loadShaderSource(std::string filename) {
	std::ifstream inFile;
	std::ostringstream sourceStream;

	inFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		inFile.open(filename);
		sourceStream << inFile.rdbuf();
		inFile.close();
	}
	catch(const std::ifstream::failure& e) {
		throw std::runtime_error("Couldn't read shader source");
	}

	return sourceStream.str();
}

void Render::setViewport(GLFWwindow* window, int width, int height) {
	Render* render = (Render*) glfwGetWindowUserPointer(window);

	glViewport(0, 0, width, height);
	render->proj = glm::perspective(ExMath::PI / 4.0f, (float)width / height, 0.1f, 10000.0f);
}
