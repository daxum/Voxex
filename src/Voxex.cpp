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

#include "Voxex.hpp"
#include "Chunk.hpp"
#include "ExtraMath.hpp"
#include "AxisAlignedBB.hpp"
#include "Perlin.hpp"
#include "ScreenComponents.hpp"
#include "AnimatedCamera.hpp"
#include "Names.hpp"
#include "DefaultCamera.hpp"
#include "ControlledAI.hpp"
#include "SquareCamera.hpp"

//TODO: This stuff goes elsewhere, most likely in its own class
namespace {
	const UniformSet chunkSet = {
		UniformSetType::MODEL_STATIC,
		1024,
		{}
	};

	const std::array<std::array<float, 3>, 20> colors = {
		0.200f, 0.200f, 0.200f, //0
		0.430f, 0.366f, 0.075f, //1
		0.100f, 0.900f, 0.150f, //2
		0.900f, 0.010f, 0.200f, //3
		0.010f, 0.300f, 0.950f, //4
		0.500f, 0.500f, 0.300f, //5
		0.300f, 0.750f, 0.800f, //6
		1.000f, 1.000f, 1.000f, //7
		1.000f, 0.000f, 0.000f, //8
		0.000f, 1.000f, 0.000f, //9
		0.000f, 0.000f, 1.000f, //10
		0.100f, 0.100f, 0.100f, //11
		0.200f, 0.200f, 0.200f, //12
		0.300f, 0.300f, 0.300f, //13
		0.400f, 0.400f, 0.400f, //14
		0.500f, 0.500f, 0.500f, //15
		0.600f, 0.600f, 0.600f, //16
		0.700f, 0.700f, 0.700f, //17
		0.800f, 0.800f, 0.800f, //18
		0.900f, 0.900f, 0.900f, //19
	};

	Chunk genChunk(const Pos_t& pos) {
		Chunk chunk(pos);

		std::stack<Aabb<int64_t>> regions;
		regions.push(Aabb<int64_t>(chunk.getBox().min, chunk.getBox().max - Pos_t(1, 1, 1)));
		uint64_t boxes = 0;

		while (!regions.empty()) {
			boxes++;

			Aabb<int64_t> box = regions.top();
			regions.pop();

			constexpr int64_t minEdge = 4;
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

		return chunk;
	}
}

void Voxex::createRenderObjects(std::shared_ptr<RenderInitializer> renderInit) {
	renderInit->createBuffer(CHUNK_BUFFER, VertexBufferInfo{{
		{VERTEX_ELEMENT_POSITION, VertexElementType::VEC3},
		{VERTEX_ELEMENT_NORMAL, VertexElementType::VEC3},
		{VERTEX_ELEMENT_COLOR, VertexElementType::VEC3}},
		BufferUsage::DEDICATED_SINGLE,
		1'073'741'824
	});

	renderInit->addUniformSet(SCREEN_SET, UniformSet{
		UniformSetType::PER_SCREEN,
		1,
		{{UniformType::MAT4, "projection", UniformProviderType::CAMERA_PROJECTION, USE_VERTEX_SHADER},
		{UniformType::MAT4, "view", UniformProviderType::CAMERA_VIEW, USE_VERTEX_SHADER}}
	});

	renderInit->addUniformSet(CHUNK_SET, chunkSet);
}

void Voxex::loadShaders(std::shared_ptr<ShaderLoader> loader) {
	ShaderInfo chunkInfo = {
		.vertex = "shaders/basicShader.vert",
		.fragment = "shaders/basicShader.frag",
		.pass = RenderPass::OPAQUE,
		.buffer = CHUNK_BUFFER,
		.uniformSets = {SCREEN_SET},
		.pushConstants = {{{UniformType::MAT4, "modelView", UniformProviderType::OBJECT_MODEL_VIEW, USE_VERTEX_SHADER}}},
	};

	loader->loadShader(CHUNK_SHADER, chunkInfo);
}

//TODO: This is not how this should be done
void Voxex::loadScreens(DisplayEngine& display) {
	std::shared_ptr<Screen> world = std::make_shared<Screen>(display, false);
	world->addComponentManager(std::make_shared<RenderComponentManager>());
	world->addComponentManager(std::make_shared<PhysicsComponentManager>());
	world->addComponentManager(std::make_shared<AIComponentManager>());

	std::vector<Chunk> chunks;
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
		Chunk chunk = genChunk(Pos_t{256*i-256*(maxI/2), 256*j-256*(maxJ/2), 256*k-256*(maxK/2)});
		double end = ExMath::getTimeMillis();

		std::cout << "Generated chunk " << val << " in " << end - start << "ms\n";

		double genExpect = 0.0;
		double genAdd = 0.0;

		do {
			genExpect = genTime.load();
			genAdd = end - start + genExpect;
		} while (!genTime.compare_exchange_strong(genExpect, genAdd, std::memory_order_relaxed));

		size_t startCount = chunk.regionCount();

		chunk.optimize();

		std::cout << "Reduced from " << startCount << " to " << chunk.regionCount() << " regions\n";

		chunk.validate();
		chunk.printStats();

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			chunks.push_back(chunk);
		}
	});

	createTime = ExMath::getTimeMillis() - createTime;

	size_t totalRegions = 0;
	size_t totalMemUsage = 0;

	for (size_t i = 0; i < chunks.size(); i++) {
		totalRegions += chunks.at(i).regionCount();
		totalMemUsage += chunks.at(i).getMemUsage();
	}

//TODO: Multithread face generation only - can't upload off the main thread in opengl
for (size_t val = 0; val < chunks.size(); val++) {
//	Engine::parallelFor(0, chunks.size(), [&](size_t val) {
		auto data = chunks.at(val).generateModel(colors);

		std::shared_ptr<Object> chunkObject = std::make_shared<Object>();

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			Engine::instance->getModelManager().addMesh(data.name, std::move(data.mesh));
			Engine::instance->getModelManager().addModel(data.name, std::move(data.model));

			chunkObject->addComponent(std::make_shared<RenderComponent>(data.name));
			glm::vec3 chunkPos = chunks.at(val).getBox().getCenter();
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

	PhysicsInfo playerPhysics = {
		.shape = PhysicsShape::SPHERE,
		.box = Aabb<float>({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}),
		.pos = glm::vec3(0.0, 1000.0, 0.0),
		.mass = 1.0f,
	};

	std::shared_ptr<Object> player = std::make_shared<Object>();
	player->addComponent(std::make_shared<PhysicsComponent>(std::make_shared<PhysicsObject>(playerPhysics)));
	player->addComponent(std::make_shared<ControlledAI>());

	world->addObject(player);

	constexpr float dist = 1800.0f;
	constexpr float center = 0.0f;
	constexpr float height = 200.0f;

	const std::vector<std::pair<glm::vec3, glm::quat>> cameraAnimation = {
		{{-dist + center, height, -center}, {1.0, 0.0, 0.0, 0.0}},
		{{center, height, dist - center}, {1.0, 0.0, 0.0, 0.0}},
		{{dist + center, height, -center}, {1.0, 0.0, 0.0, 0.0}},
		{{center, height, -dist - center}, {1.0, 0.0, 0.0, 0.0}},
		{{-dist + center, height, -center}, {1.0, 0.0, 0.0, 0.0}},
		{{center, height, dist - center}, {1.0, 0.0, 0.0, 0.0}},
		{{dist + center, height, -center}, {1.0, 0.0, 0.0, 0.0}}
	};

	//world->setCamera(std::make_shared<AnimatedCamera>(cameraAnimation, 2000));
	std::shared_ptr<SquareCamera> camera = std::make_shared<SquareCamera>();
	camera->setTarget(player);
	world->setCamera(camera);

	display.pushScreen(world);
}
