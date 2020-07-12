/******************************************************************************
 * Voxex - An experiment with sparse voxel terrain
 * Copyright (C) 2019, 2020
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

#include <atomic>
#include <mutex>
#include <stack>
#include <fstream>

#include "Voxex.hpp"
#include "ScreenComponents.hpp"
#include "AnimatedCamera.hpp"
#include "Names.hpp"
#include "ChunkLoader.hpp"
#include "Mobs/Adventurer.hpp"
#include "Mobs/BoxMonster.hpp"
#include "FollowCamera.hpp"
#include "MouseHandler.hpp"

namespace {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
	void writeObj(const std::string& outFile, const Mesh& mesh) {
		std::ofstream out(outFile + ".obj");
		const unsigned char* vertData = std::get<0>(mesh.getMeshData());
		size_t vertSize = std::get<1>(mesh.getMeshData());
		const std::vector<uint32_t>& indices = std::get<2>(mesh.getMeshData());

		out << "o " << outFile << "\n";

		for (size_t i = 0; i < vertSize / 16; i++) {
			glm::vec3 vert = *(glm::vec3*)&vertData[i*16];
			out << "v " << vert.x << " " << vert.y << " " << vert.z << "\n";
		}

		for (size_t i = 0; i < indices.size(); i += 3) {
			out << "f " << (indices.at(i) + 1) << "// " << (indices.at(i+1) + 1) << "// " << (indices.at(i+2) + 1) << "//\n";
		}
	}
#pragma GCC diagnostic pop
}

namespace {
	struct ShaderNames {
		const char* const vertex;
		const char* const fragment;
	};

	struct ShaderPaths {
		ShaderNames chunk;
		ShaderNames basic;
	};

	const ShaderPaths glShaders = {
		{"shaders/glsl/chunk.vert", "shaders/glsl/chunk.frag"},
		{"shaders/glsl/generic.vert", "shaders/glsl/basic.frag"}
	};

	const ShaderPaths vkShaders = {
		{"shaders/spirv/chunk.vert.spv", "shaders/spirv/chunk.frag.spv"},
		{"shaders/spirv/generic.vert.spv", "shaders/spirv/basic.frag.spv"}
	};

	const ShaderPaths* const shaderFiles = Voxex::USE_VULKAN ? &vkShaders : &glShaders;

}

void Voxex::createRenderObjects(RenderInitializer& renderInit) {
	const size_t chunkBufferSize = 1'073'741'824;
	const size_t indexSize = (4 * chunkBufferSize) / 6;

	renderInit.createBuffer(CHUNK_VERTEX_BUFFER, chunkBufferSize, BufferType::VERTEX, BufferStorage::DEVICE);
	renderInit.createBuffer(CHUNK_INDEX_BUFFER, indexSize, BufferType::INDEX, BufferStorage::DEVICE);
	renderInit.createBuffer(GENERIC_VERTEX_BUFFER, 1'048'576, BufferType::VERTEX, BufferStorage::DEVICE);
	renderInit.createBuffer(GENERIC_INDEX_BUFFER, 1'048'576, BufferType::INDEX, BufferStorage::DEVICE);

	renderInit.addVertexFormat(CHUNK_FORMAT, VertexFormat({
		{VERTEX_ELEMENT_POSITION, VertexFormat::ElementType::VEC3},
		{VERTEX_ELEMENT_PACKED_NORM_COLOR, VertexFormat::ElementType::UINT32}
	}));

	renderInit.addVertexFormat(GENERIC_FORMAT, VertexFormat({
		{VERTEX_ELEMENT_POSITION, VertexFormat::ElementType::VEC3},
		{VERTEX_ELEMENT_NORMAL, VertexFormat::ElementType::VEC3},
		{VERTEX_ELEMENT_TEXTURE, VertexFormat::ElementType::VEC2}
	}));

	renderInit.addUniformSet(SCREEN_SET, UniformSetType::PER_SCREEN, 2,
		{{UniformType::MAT4, "projection", 0, UniformProviderType::CAMERA_PROJECTION, USE_VERTEX_SHADER},
		{UniformType::MAT4, "view", 0, UniformProviderType::CAMERA_VIEW, USE_VERTEX_SHADER}}
	);

	renderInit.addUniformSet(CHUNK_SET, UniformSetType::MATERIAL, 1, {
		{UniformType::SAMPLER_2D, UNIFORM_NAME_KD_TEX, 0, UniformProviderType::MATERIAL, USE_FRAGMENT_SHADER},
	});

	renderInit.addUniformSet(BASIC_SET, UniformSetType::MATERIAL, 1, {
		{UniformType::SAMPLER_2D, UNIFORM_NAME_KD_TEX, 0, UniformProviderType::MATERIAL, USE_FRAGMENT_SHADER},
	});
}

void Voxex::loadTextures(std::shared_ptr<TextureLoader> loader) {
	loader->loadTexture(TEST_TEX, "textures/test.png", Filter::LINEAR, Filter::LINEAR, true);
	loader->loadTexture(TERRAIN_TEX, "textures/terrain.png", Filter::NEAREST, Filter::NEAREST, true);
}

void Voxex::loadModels(ModelLoader& loader) {
	MaterialCreateInfo chunkMat = {
		.filename = "models/chunk.mtl",
		.shader = CHUNK_SHADER,
		.uniformSet = CHUNK_SET,
		.viewCull = true,
	};

	MaterialCreateInfo playerMat = {
		.filename = "models/capsule.mtl",
		.shader = BASIC_SHADER,
		.uniformSet = BASIC_SET,
		.viewCull = true,
	};

	loader.loadMaterial(CHUNK_MAT, chunkMat);
	loader.loadMaterial(PLAYER_MAT, playerMat);

	MeshCreateInfo playerMesh = {
		.filename = "models/capsule.obj",
		.vertexBuffer = GENERIC_VERTEX_BUFFER,
		.indexBuffer = GENERIC_INDEX_BUFFER,
		.vertexFormat = GENERIC_FORMAT,
		.renderable = true,
	};

	MeshCreateInfo selectMesh = {
		.filename = "models/cube.obj",
		.vertexBuffer = GENERIC_VERTEX_BUFFER,
		.indexBuffer = GENERIC_INDEX_BUFFER,
		.vertexFormat = GENERIC_FORMAT,
		.renderable = true,

	};

	loader.loadMesh(PLAYER_MESH, playerMesh);
	loader.loadMesh(SELECT_MESH, selectMesh);
}

void Voxex::loadShaders(std::shared_ptr<ShaderLoader> loader) {
	ShaderInfo chunkInfo = {
		.vertex = shaderFiles->chunk.vertex,
		.fragment = shaderFiles->chunk.fragment,
		.pass = RenderPass::OPAQUE,
		.format = CHUNK_FORMAT,
		.uniformSets = {SCREEN_SET, CHUNK_SET},
		.pushConstants = {{UniformType::MAT4, "modelView", 0, UniformProviderType::OBJECT_MODEL_VIEW, USE_VERTEX_SHADER}},
	};

	ShaderInfo basicInfo = {
		.vertex = shaderFiles->basic.vertex,
		.fragment = shaderFiles->basic.fragment,
		.pass = RenderPass::OPAQUE,
		.format = GENERIC_FORMAT,
		.uniformSets = {SCREEN_SET, BASIC_SET},
		.pushConstants = {{UniformType::MAT4, "modelView", 0, UniformProviderType::OBJECT_MODEL_VIEW, USE_VERTEX_SHADER}},
	};

	loader->loadShader(CHUNK_SHADER, chunkInfo);
	loader->loadShader(BASIC_SHADER, basicInfo);
}

void Voxex::loadScreens(DisplayEngine& display) {
	std::shared_ptr<Screen> world = std::make_shared<Screen>(display, false);
	world->addComponentManager<RenderManager>();
	world->addComponentManager<PhysicsManager>();
	world->addComponentManager<AIManager>();
	world->addComponentManager<UpdateManager>();

	std::shared_ptr<Object> chunkLoader = std::make_shared<Object>();
	chunkLoader->addComponent<ChunkLoader>();
	//chunkLoader->getComponent<ChunkLoader>()->addLoader(chunkLoader, 1);

	world->addObject(chunkLoader);

	for (size_t i = 0; i < 10; i++) {
		world->addObject(BoxMonster::create({0.0, 300.0 + i + 0.5, 0.0}));
	}

	std::shared_ptr<Object> player = Adventurer::create();
	chunkLoader->getComponent<ChunkLoader>()->addLoader(player, 1, 3);

	world->addObject(player);

	std::shared_ptr<Object> selectBox = std::make_shared<Object>();
	selectBox->addComponent<MouseHandler>(chunkLoader->getComponent<ChunkLoader>());
	selectBox->addComponent<RenderComponent>(PLAYER_MAT, SELECT_MESH);

	world->addObject(selectBox);
	world->setCamera(std::make_shared<FollowCamera>(player));

	display.pushScreen(world);
}
