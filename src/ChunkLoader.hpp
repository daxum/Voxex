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

#include <memory>
#include <vector>
#include <unordered_map>

#include <tbb/concurrent_queue.h>

#include "Components/UpdateComponent.hpp"
#include "Chunk.hpp"

class ChunkLoader : public UpdateComponent {
public:
	ChunkLoader() : tick(0) {}

	/**
	 * Refreshes which chunks should be loaded, unloads uneeded chunks,
	 * asynchronously generates chunks, etc.
	 * @param screen The parent screen.
	 */
	void update(Screen* screen) override;

	/**
	 * Adds a chunk loader to the world.
	 * @param The object for which the chunks are loaded.
	 * @param critDist The range of chunks which must be loaded at all times.
	 * @param prefDist The range of chunks which should be loaded for gameplay smoothness.
	 */
	void addLoader(std::shared_ptr<Object> object, uint64_t critDist, uint64_t prefDist) {
		chunkLoaders.push_back({object, critDist, prefDist});
	}

	/**
	 * Returns the chunk the given position is inside, preferring the chunk farther
	 * from zero if on a border.
	 * @param pos The position to get the chunk for, in world coordinates.
	 * @return The chunk at the given position, or nullptr if none is loaded.
	 */
	std::shared_ptr<Chunk> getChunk(glm::vec3 pos);

private:
	struct PosHash {
		size_t operator()(const Pos_t& pos) const noexcept {
			uint64_t x = pos.x;
			uint64_t y = pos.y;
			uint64_t z = pos.z;
			return (((x >> 1) ^ y) << 1) ^ z;
		}
	};

	struct LoaderObj {
		std::weak_ptr<Object> loader;
		uint64_t critRange;
		uint64_t prefRange;
	};

	//Current tick, used for determining which chunks to unload.
	size_t tick;
	//All currently loaded chunks, sorted by position.
	std::unordered_map<Pos_t, std::shared_ptr<Chunk>, PosHash> chunkMap;
	//Stores all currently loaded chunks.
	std::vector<std::shared_ptr<Chunk>> loadedChunks;
	//All objects capable of loading chunks, as well as the radius of the
	//box they should load.
	std::vector<LoaderObj> chunkLoaders;
	//Chunks which have finished generation and can be added to the map.
	tbb::concurrent_queue<std::shared_ptr<Chunk>> completeChunks;

	/**
	 * Adds a chunk to the loader's internal data structure, as well as
	 * the provided screen.
	 * @param screen The screen to add the chunk to.
	 * @param chunk The chunk to add to the screen.
	 */
	void addChunk(Screen* screen, std::shared_ptr<Chunk> chunk);

	/**
	 * Function used to asynchronously generate a chunk.
	 * @param pos The chunk to generate.
	 */
	void dispatchChunkGen(const Pos_t& pos);

	/**
	 * Temporary function to generate a chunk using the given position.
	 * @param pos the position of the corner of the chunk closest to the
	 *     origin.
	 * @return The generated chunk.
	 */
	std::shared_ptr<Chunk> genChunk(const Pos_t& pos);
};
