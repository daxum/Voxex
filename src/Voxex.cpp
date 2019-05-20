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
	};

	const ShaderPaths glShaders = {
		{"shaders/glsl/chunk.vert", "shaders/glsl/chunk.frag"}
	};

	const ShaderPaths vkShaders = {
		{"shaders/spirv/chunk.vert.spv", "shaders/spirv/chunk.frag.spv"}
	};

	const ShaderPaths* const shaderFiles = Voxex::USE_VULKAN ? &vkShaders : &glShaders;

}

void Voxex::createRenderObjects(RenderInitializer& renderInit) {
	const size_t chunkBufferSize = 1'073'741'824;
	const size_t indexSize = (4 * chunkBufferSize) / 6;

	renderInit.createBuffer(CHUNK_VERTEX_BUFFER, chunkBufferSize, BufferType::VERTEX, BufferStorage::DEVICE);
	renderInit.createBuffer(CHUNK_INDEX_BUFFER, indexSize, BufferType::INDEX, BufferStorage::DEVICE);

	renderInit.addVertexFormat(CHUNK_FORMAT, VertexFormat({
		{VERTEX_ELEMENT_POSITION, VertexFormat::ElementType::VEC3},
		{VERTEX_ELEMENT_PACKED_NORM_COLOR, VertexFormat::ElementType::UINT32}
	}));

	renderInit.addUniformSet(SCREEN_SET, UniformSetType::PER_SCREEN, 1,
		{{UniformType::MAT4, "projection", UniformProviderType::CAMERA_PROJECTION, USE_VERTEX_SHADER},
		{UniformType::MAT4, "view", UniformProviderType::CAMERA_VIEW, USE_VERTEX_SHADER}}
	);

	renderInit.addUniformSet(CHUNK_SET, UniformSetType::MATERIAL, 1, {});
}

void Voxex::loadModels(ModelLoader& loader) {
	//Chunks don't have anything in their material at the moment, so skip standard loading
	//This will be fixed later!
	const UniformSet& chunkSet = Engine::instance->getModelManager().getMemoryManager()->getUniformSet(CHUNK_SET);
	Engine::instance->getModelManager().addMaterial(CHUNK_MAT, Material(CHUNK_MAT, CHUNK_SHADER, CHUNK_SET, chunkSet));
}

void Voxex::loadShaders(std::shared_ptr<ShaderLoader> loader) {
	ShaderInfo chunkInfo = {
		.vertex = shaderFiles->chunk.vertex,
		.fragment = shaderFiles->chunk.fragment,
		.pass = RenderPass::OPAQUE,
		.format = CHUNK_FORMAT,
		.uniformSets = {SCREEN_SET, CHUNK_SET},
		.pushConstants = {{UniformType::MAT4, "modelView", UniformProviderType::OBJECT_MODEL_VIEW, USE_VERTEX_SHADER}},
	};

	loader->loadShader(CHUNK_SHADER, chunkInfo);
}

//TODO: This is not how this should be done
void Voxex::loadScreens(DisplayEngine& display) {
	std::shared_ptr<Screen> world = std::make_shared<Screen>(display, false);
	world->addComponentManager<RenderComponentManager>();
	world->addComponentManager<PhysicsComponentManager>();
	world->addComponentManager<AIComponentManager>();
	world->addComponentManager<UpdateComponentManager>();

	std::shared_ptr<Object> chunkLoader = std::make_shared<Object>();
	chunkLoader->addComponent<ChunkLoader>();
	//chunkLoader->getComponent<ChunkLoader>()->addLoader(chunkLoader, 1);

	world->addObject(chunkLoader);

	for (size_t i = 0; i < 10; i++) {
		world->addObject(BoxMonster::create({0.0, 300.0 + i + 0.5, 0.0}));
	}

	std::shared_ptr<Object> player = Adventurer::create();
	chunkLoader->getComponent<ChunkLoader>()->addLoader(player, 1);

	world->addObject(player);
	world->setCamera(std::make_shared<FollowCamera>(player));

	display.pushScreen(world);
}
