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
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <tbb/concurrent_queue.h>

#include "Components/UpdateComponent.hpp"
#include "Chunk.hpp"

class ChunkLoader : public UpdateComponent {
public:
	/**
	 * Initializes values and starts the chunk generator thread.
	 */
	ChunkLoader();

	/**
	 * Sends the stop signal to the worker thread and flushes all chunks
	 * to disk, if needed.
	 */
	~ChunkLoader();

	/**
	 * Refreshes which chunks should be loaded, unloads uneeded chunks,
	 * asynchronously generates chunks, etc.
	 * @param screen The parent screen.
	 */
	void update(Screen* screen) override;

	/**
	 * Adds a chunk loader to the world.
	 * @param The object for which the chunks are loaded.
	 * @param dist The radius at which chunks are loaded.
	 */
	void addLoader(std::shared_ptr<Object> object, uint64_t dist) {
		chunkLoaders.emplace_back(object, dist);
	}

private:
	struct PosHash {
		size_t operator()(const Pos_t& pos) const noexcept {
			uint64_t x = pos.x;
			uint64_t y = pos.y;
			uint64_t z = pos.z;
			return (((x >> 1) ^ y) << 1) ^ z;
		}
	};

	//All currently loaded chunks, sorted by position.
	std::unordered_map<Pos_t, std::shared_ptr<Chunk>, PosHash> chunkMap;
	//Stores all currently loaded chunks.
	std::vector<std::shared_ptr<Chunk>> loadedChunks;
	//All objects capable of loading chunks, as well as the radius of the
	//box they should load.
	std::vector<std::pair<std::weak_ptr<Object>, uint64_t>> chunkLoaders;

	//Chunk generation thread.
	std::thread genThread;
	//Signals that generation should stop (generation thread should exit).
	std::atomic<bool> genStop;
	//Used to sleep until chunks need to be generated.
	std::condition_variable genWait;
	//Needed for condition variable.
	std::mutex genLock;
	//Chunks which have finished generation and can be added to the map.
	tbb::concurrent_queue<std::shared_ptr<Chunk>> completeChunks;
	//Queue of positions to generate.
	tbb::concurrent_queue<Pos_t> genPos;

	/**
	 * Adds a chunk to the loader's internal data structure, as well as
	 * the provided screen.
	 * @param screen The screen to add the chunk to.
	 * @param chunk The chunk to add to the screen.
	 */
	void addChunk(Screen* screen, std::shared_ptr<Chunk> chunk);

	/**
	 * Function ran by the chunk generator thread.
	 */
	void genChunkWorker();

	/**
	 * Temporary function to generate a chunk using the given position.
	 * @param pos the position of the corner of the chunk closest to the
	 *     origin.
	 * @return The generated chunk.
	 */
	std::shared_ptr<Chunk> genChunk(const Pos_t& pos);
};
