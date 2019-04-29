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

#include <vector>

#include "RegionTree.hpp"

class BlockMap {
public:
	/**
	 * Creates a map for representing the filled areas in a chunk.
	 * @param posOffset Offset into the chunk to begin the map.
	 * @param mapDims The size of the map.
	 */
	BlockMap(Aabb<uint64_t>::vec_t posOffset = {0, 0, 0}, Aabb<uint64_t>::vec_t mapDims = {256, 256, 256}) :
		map(mapDims.x * mapDims.y * mapDims.z, false),
		mapBox(posOffset, posOffset + mapDims),
		mapDims(mapDims) {}

	/**
	 * Sets the given postion as filled or empty in the map.
	 * @param x,y,z The position to set.
	 * @param val Whether to set the block as filled or empty.
	 */
	void setBlockFill(size_t x, size_t y, size_t z, bool val = true) {
		map.at(x * mapDims.y * mapDims.z + y * mapDims.z + z) = val;
	}

	/**
	 * Gets whether the block at the given position is filled.
	 * @param x,y,z The position to check.
	 * @return Whether the given block was filled.
	 */
	bool isBlockFilled(size_t x, size_t y, size_t z) const {
		return map.at(x * mapDims.y * mapDims.z + y * mapDims.z + z);
	}

	/**
	 * Sets the area encompassed by the region as filled.
	 * @param region The region to set as filled.
	 */
	void addRegionFill(const InternalRegion& region) {
		Aabb<uint64_t> regBox(region.box);
		regBox.max += Aabb<uint64_t>::vec_t(1, 1, 1);

		if (!mapBox.intersects(regBox)) {
			return;
		}

		Aabb<uint64_t>::vec_t min = glm::max(mapBox.min, regBox.min);
		Aabb<uint64_t>::vec_t max = glm::min(mapBox.max, regBox.max);

		uint64_t xInc = mapDims.y * mapDims.z;
		uint64_t xMin = min.x * mapDims.y * mapDims.z;
		uint64_t xMax = max.x * mapDims.y * mapDims.z;
		uint64_t yMin = min.y * mapDims.z;
		uint64_t yMax = max.y * mapDims.z;

		for (uint64_t x = xMin; x < xMax; x += xInc) {
			for (uint64_t y = yMin; y < yMax; y += mapDims.z) {
				for (uint64_t z = min.z; z < max.z; z++) {
					map.at(x + y + z) = true;
				}
			}
		}
	}

	/**
	 * Checks whether the face is visible based on the internal map. Faces
	 * on the edge of the map are always counted as visible.
	 * @param face The face to check.
	 * @return Whether the face is visible.
	 */
	bool isFaceVisible(const RegionFace& face) const {
		uint64_t fixed = face.getFixedCoord();
		uint64_t normal = face.getNormal();
		bool visible = false;

		//Check if face is actually in the map
		bool inMap = false;
		glm::uvec2 minBound(0, 0);
		glm::uvec2 maxBound(0, 0);

		switch (normal) {
			//-Z
			case 0: inMap = fixed > mapBox.min.z; minBound = {mapBox.min.x, mapBox.min.y}; maxBound = {mapBox.max.x, mapBox.max.y}; break;
			//+X
			case 1: inMap = fixed < mapBox.max.x; minBound = {mapBox.min.y, mapBox.min.z}; maxBound = {mapBox.max.y, mapBox.max.z}; break;
			//+Z
			case 2: inMap = fixed < mapBox.max.z; minBound = {mapBox.min.x, mapBox.min.y}; maxBound = {mapBox.max.x, mapBox.max.y}; break;
			//-X
			case 3: inMap = fixed > mapBox.min.x; minBound = {mapBox.min.y, mapBox.min.z}; maxBound = {mapBox.max.y, mapBox.max.z}; break;
			//+Y
			case 4: inMap = fixed < mapBox.max.y; minBound = {mapBox.min.x, mapBox.min.z}; maxBound = {mapBox.max.x, mapBox.max.z}; break;
			//-Y
			case 5: inMap = fixed > mapBox.min.y; minBound = {mapBox.min.x, mapBox.min.z}; maxBound = {mapBox.max.x, mapBox.max.z}; break;
			default: throw std::runtime_error("Bad switch in FillMap::faceVisible");
		}

		bool biggerThanMap = face.min[0] < minBound.x || face.max[0] > maxBound.x || face.min[1] < minBound.y || face.max[1] > maxBound.y;

		if (!inMap || biggerThanMap) {
			return true;
		}

		for (uint64_t i = face.min.at(0); i < face.max.at(0); i++) {
			for (uint64_t j = face.min.at(1); j < face.max.at(1); j++) {
				//Should offset the positive directions (+x, +y, +z, or east, up, south)
				switch (normal) {
					//North, -z. i=x, j=y, fixed=z
					case 0: visible |= !isBlockFilled(i, j, fixed - 1); break;
					//East, +x. i=y, j=z, fixed=x
					case 1: visible |= !isBlockFilled(fixed, i, j); break;
					//South, +z. i=x, j=y, fixed=z
					case 2: visible |= !isBlockFilled(i, j, fixed); break;
					//West, -x. i=y, j=z, fixed=x
					case 3: visible |= !isBlockFilled(fixed - 1, i, j); break;
					//Up, +y. i=x, j=z, fixed=y
					case 4: visible |= !isBlockFilled(i, fixed, j); break;
					//Down, -y. i=x, j=z, fixed=y
					case 5: visible |= !isBlockFilled(i, fixed - 1, j); break;
					default: throw std::runtime_error("Bad switch in FillMap::faceVisible");
				}

				if (visible) {
					return visible;
				}
			}
		}

		return visible;
	}

	/**
	 * Gets the map of covered blocks.
	 * @return The block coverage map.
	 */
	const std::vector<bool>& getMap() const { return map; }

private:
	//Stores which blocks are filled in the chunk.
	std::vector<bool> map;
	//Bounding box for the start of the map.
	Aabb<uint64_t> mapBox;
	//Size of the map, same as mapBox.max - mapBox.min.
	Aabb<uint64_t>::vec_t mapDims;
};
