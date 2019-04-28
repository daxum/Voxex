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
#include "ChunkBuilder.hpp"
#include "ExtraMath.hpp"
#include "AxisAlignedBB.hpp"
#include "Perlin.hpp"
#include "ScreenComponents.hpp"
#include "AnimatedCamera.hpp"
#include "Names.hpp"
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

	std::shared_ptr<Chunk> worldGenChunk(const Pos_t& pos) {
		const std::string seed = "WorldMaker";

		ChunkBuilder chunk(pos);
		Aabb<int64_t> chunkBox = chunk.getBox();

		//Ground - anything below 0 is underground
		if (chunkBox.max.y <= 0) {
			Aabb<int64_t> groundBox = chunkBox;

			chunk.addRegion({16, groundBox});
		}

		//Add layer of dirt and stone for terrain
		else if (chunkBox.max.y <= 256 && chunkBox.min.y >= 0) {
			for (int64_t i = pos.x; i < chunkBox.max.x; i++) {
				for (int64_t j = pos.z; j < chunkBox.max.z; j++) {
					float heightPercent = perlin2DOctaves({i, j}, 8, 512, std::hash<std::string>()(seed));
					int64_t height = (int64_t) (heightPercent * 255) + pos.y;
					int64_t stoneHeight = pos.y + height / 2;

					if (stoneHeight > 0) {
						chunk.addRegion({16, Aabb<int64_t>({i, 0, j}, {i+1, stoneHeight, j+1})});
					}

					chunk.addRegion({2, Aabb<int64_t>({i, stoneHeight, j}, {i+1, height, j+1})});
				}
			}
		}

		return chunk.genChunk();
	}
#pragma GCC diagnostic pop
}

//TODO: This stuff goes elsewhere, most likely in its own class
namespace {
	std::shared_ptr<Chunk> genChunk(const Pos_t& pos) {
		ChunkBuilder chunk(pos);

		std::stack<Aabb<int64_t>> regions;
		regions.push(Aabb<int64_t>(chunk.getBox().min, chunk.getBox().max));

		while (!regions.empty()) {
			Aabb<int64_t> box = regions.top();
			regions.pop();

			constexpr int64_t minEdge = 1;
			constexpr float fillThreshold = 0.20f;
			constexpr float cutoffScale = 72.0f;
			constexpr float discardThreshold = 0.37f;

			float percentFull = perlin3D(box.getCenter(), 256);
			float adjThreshold = ExMath::clamp(cutoffScale * (1.0f / (256.0f - box.xLength())) + fillThreshold, 0.0f, 0.95f);

			if (percentFull >= adjThreshold) {
				uint16_t type = 5;//(uint16_t)ExMath::randomInt(0, colors.size() - 1);
				chunk.addRegion(Region{type, box});
			}
			else if (box.xLength() > minEdge && percentFull > discardThreshold) {
				std::array<Aabb<int64_t>, 8> toAdd = box.split();

				for (Aabb<int64_t> add : toAdd) {
					if (add.getVolume() > 0) {
						regions.push(add);
					}
				}
			}
		}

		return chunk.genChunk();
	}

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

	renderInit.createBuffer(CHUNK_VERTEX_BUFFER, chunkBufferSize, BufferType::VERTEX, BufferStorage::DEVICE);
	renderInit.createBuffer(CHUNK_INDEX_BUFFER, chunkBufferSize / 6, BufferType::INDEX, BufferStorage::DEVICE);

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
	world->addComponentManager(std::make_shared<RenderComponentManager>());
	world->addComponentManager(std::make_shared<PhysicsComponentManager>());
	world->addComponentManager(std::make_shared<AIComponentManager>());
	world->addComponentManager(std::make_shared<UpdateComponentManager>());

	std::vector<std::shared_ptr<Chunk>> chunks;
	std::mutex chunkLock;

	constexpr size_t maxI = 4;
	constexpr size_t maxJ = 4;
	constexpr size_t maxK = 4;
	constexpr size_t maxChunks = maxI * maxJ * maxK;

	std::atomic<double> genTime(0.0);
	double createTime = ExMath::getTimeMillis();

	Engine::parallelFor(0, maxChunks, [&](size_t val) {
		size_t i = (val / (maxK * maxJ)) % maxI;
		size_t j = val / maxK % maxJ;
		size_t k = val % maxK;

		double start = ExMath::getTimeMillis();
		std::shared_ptr<Chunk> chunk = worldGenChunk(Pos_t{256*i-256*(maxI/2), 256*j-256*(maxJ/2), 256*k-256*(maxK/2)});
		double end = ExMath::getTimeMillis();

		std::cout << "Generated chunk " << val << " in " << end - start << "ms\n";

		double genExpect = 0.0;
		double genAdd = 0.0;

		do {
			genExpect = genTime.load();
			genAdd = end - start + genExpect;
		} while (!genTime.compare_exchange_strong(genExpect, genAdd, std::memory_order_relaxed));

		chunk->validate();
		chunk->printStats();

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			chunks.push_back(chunk);
		}
	});

	createTime = ExMath::getTimeMillis() - createTime;

	size_t totalRegions = 0;
	size_t totalMemUsage = 0;

	for (size_t i = 0; i < chunks.size(); i++) {
		totalRegions += chunks.at(i)->regionCount();
		totalMemUsage += chunks.at(i)->getMemUsage();
	}

//TODO: Multithread face generation only - can't upload off the main thread in opengl
for (size_t val = 0; val < chunks.size(); val++) {
//	Engine::parallelFor(0, chunks.size(), [&](size_t val) {
		if (chunks.at(val)->regionCount() == 0) {
			continue;
		}

		auto data = chunks.at(val)->generateMesh();
		//writeObj(data.model.name, data.mesh);

		std::shared_ptr<Object> chunkObject = std::make_shared<Object>();

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			Engine::instance->getModelManager().addMesh(data.name, std::move(data.mesh), false);

			chunkObject->addComponent<RenderComponent>(CHUNK_MAT, data.name);
			glm::vec3 chunkPos = chunks.at(val)->getBox().getCenter();
			chunkPos.z = -chunkPos.z;
			chunkObject->addComponent(std::make_shared<PhysicsComponent>(std::make_shared<PhysicsObject>(data.name, chunkPos)));
		}

		world->addObject(chunkObject);

		std::cout << "Chunk " << val << " complete!\n";
	}//);

	std::cout << "Generated in " << genTime.load() << "ms (" << genTime.load() / (maxI*maxJ*maxK) << "ms per chunk)!\n";
	std::cout << "Generation completed in " << createTime << "ms\n";
	std::cout << "Generated " << totalRegions << " regions\n";
	//Yes, I know the correct term
	std::cout << "Chunks using around " << (totalMemUsage >> 10) << " kilobytes in total\n";

	for (size_t i = 0; i < 10; i++) {
		world->addObject(BoxMonster::create({0.0, 300.0 + i + 0.5, 0.0}));
	}

	std::shared_ptr<Object> player = Adventurer::create();

	world->addObject(player);
	world->setCamera(std::make_shared<FollowCamera>(player));

	display.pushScreen(world);
}
