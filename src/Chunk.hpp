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

#include "AxisAlignedBB.hpp"
#include "RegionTree.hpp"
#include "Model.hpp"

typedef Aabb<int64_t>::vec_t Pos_t;

struct Region {
	uint16_t type;
	Aabb<int64_t> box;
};

struct ChunkMeshData {
	std::string name;
	Mesh mesh;
	Model model;
};

class Chunk {
public:
	Chunk(Pos_t pos) : regions(512), box(pos, pos + Pos_t{256, 256, 256}) {}

	/**
	 * Adds a region to the chunk, without overwriting old regions.
	 * @param reg The region to add.
	 */
	void addRegion(const Region& reg);

	/**
	 * Generates a mesh from this chunk using the specific colors for its regions.
	 * @param colors The colors for the regions.
	 * @return The chunk's mesh data.
	 */
	ChunkMeshData generateModel(const std::array<std::array<float, 3>, 20>& colors);

	/**
	 * Returns the number of regions in the chunk.
	 * @return The number of regions.
	 */
	size_t regionCount() { return regions.size(); }

	/**
	 * Prints out number of regions per node for the region tree,
	 * number of nodes, and other info.
	 */
	void printStats();

	/**
	 * Attempts to minimize the number of regions in the chunk by merging
	 * adjacent boxes with the same type.
	 * This may be rendered unneccessary by better region storage later.
	 */
	void optimize() { regions.optimizeTree(); }

	/**
	 * Checks whether any regions in the chunk are overlapping.
	 */
	void validate() const {
		//TODO
	}

	/**
	 * Gets the bounding box for the chunk.
	 * @return The chunk's box.
	 */
	Aabb<int64_t> getBox() const { return box; }

	/**
	 * Calculates how much memory the chunk is using.
	 * @return The chunk's memory usage, in bytes.
	 */
	size_t getMemUsage() { return sizeof(Chunk) - sizeof(RegionTree) + regions.getMemUsage(); }

private:
	//List of regions in the chunk.
	RegionTree regions;
	//Chunk bounding box.
	Aabb<int64_t> box;
};
