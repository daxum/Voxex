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

#include <stack>

#include "ChunkLoader.hpp"
#include "ChunkBuilder.hpp"
#include "Perlin.hpp"
#include "ScreenComponents.hpp"
#include "Names.hpp"

void ChunkLoader::update(Screen* screen) {
	for (size_t i = 0; i < chunkLoaders.size(); i++) {
		std::shared_ptr<Object> loader = chunkLoaders.at(i).first.lock();

		if (!loader) {
			chunkLoaders.at(i) = chunkLoaders.back();
			chunkLoaders.pop_back();
			i--;
			continue;
		}

		glm::vec3 pos = loader->getPhysics()->getTranslation();

		Pos_t centerChunk = pos;
		centerChunk = centerChunk - 256l * Pos_t(glm::lessThan(pos, glm::vec3(0.0f)));
		centerChunk /= 256l;

		int64_t loadRadius = chunkLoaders.at(i).second;
		Aabb<int64_t> loadBox(centerChunk - loadRadius, centerChunk + loadRadius);

		for (int64_t x = loadBox.min.x; x <= loadBox.max.x; x++) {
			for (int64_t y = loadBox.min.y; y <= loadBox.max.y; y++) {
				for (int64_t z = loadBox.min.z; z <= loadBox.max.z; z++) {
					Pos_t chunkPos(x, y, -z);

					if (!chunkMap.count(chunkPos)) {
						std::cout << "Generating chunk (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")\n";

						std::shared_ptr<Chunk> chunk = genChunk(chunkPos * 256l);
						loadedChunks.push_back(chunk);
						chunkMap.emplace(chunkPos, chunk);

						if (chunk->regionCount() != 0) {
							auto data = chunk->generateMesh();

							std::shared_ptr<Object> chunkObject = std::make_shared<Object>();

							Engine::instance->getModelManager().addMesh(data.name, std::move(data.mesh), false);

							chunkObject->addComponent<RenderComponent>(CHUNK_MAT, data.name);
							glm::vec3 blockPos(chunkPos * 256l);
							blockPos.z = -blockPos.z;
							chunkObject->addComponent<PhysicsComponent>(std::make_shared<PhysicsObject>(data.name, blockPos));

							screen->addObject(chunkObject);
						}
					}
				}
			}
		}
	}
}

std::shared_ptr<Chunk> ChunkLoader::genChunk(const Pos_t& pos) {
#if 0
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
			uint16_t type = 5;//(uint16_t)ExMath::randomInt(0, 19);
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
#else
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
#endif
}
