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
#include <unordered_map>

#include "Chunk.hpp"

class ChunkBuilder {
public:
	/**
	 * Creates a chunk builder, which will generate a chunk at the given position.
	 */
	ChunkBuilder(Pos_t pos) : box(pos, pos + Pos_t(256, 256, 256)) {}

	/**
	 * Queues a region to be added to the generated chunk.
	 * @param reg The region to add.
	 */
	void addRegion(const Region& reg);

	/**
	 * Gets the chunk's bounding box.
	 * @return The bounding box for the chunk.
	 */
	const Aabb<int64_t>& getBox() const { return box; }

	/**
	 * Generates the chunk.
	 * @return The generated chunk.
	 */
	std::shared_ptr<Chunk> genChunk() const;

private:
	//Sorts regions by type, for hopefully faster chunk optimization.
	std::unordered_map<uint16_t, std::vector<InternalRegion>> sortedRegions;
	//Chunk bounding box.
	Aabb<int64_t> box;
};
