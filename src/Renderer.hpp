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

#pragma once

#include <string>

#include <glm/glm.hpp>

#include "CombinedGl.h"
#include "SplineAnimation.hpp"
#include "Chunk.hpp"

class Render {
public:
	Render();
	~Render();

	void render();
	void present();
	bool shouldExit() { return glfwWindowShouldClose(window); }
	void pollEvents() { glfwPollEvents(); }
	void addChunkData(const std::pair<std::vector<ChunkVertex>, std::vector<uint32_t>>& chunkData);
	void upload() { initBuffers(); }

private:
	GLFWwindow* window;
	GLuint prog;
	glm::mat4 proj;
	GLuint vao;
	GLuint vertexBuffer;
	GLuint indexBuffer;

	size_t indexCount = 0;
	std::vector<float> renderBoxes;
	std::vector<uint32_t> renderIndices;

	glm::vec3 camPos;
	glm::vec3 camLook;
	glm::vec3 camUp;

	SplineAnimation spline;

	void init();
	void terminate();
	void loadShaders();
	void initBuffers();

	std::string loadShaderSource(std::string filename);
	GLuint createShader(std::string filename, GLenum type);

	static void setViewport(GLFWwindow* window, int width, int height);
};
