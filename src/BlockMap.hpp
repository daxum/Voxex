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

#include <bitset>

#include "RegionTree.hpp"

class BlockMap {
public:
	static constexpr size_t length = 256;
	static constexpr size_t area = length * length;
	static constexpr size_t volume = area * length;

	//Calculated as log2(length)
	static constexpr size_t lengthShift = 8;
	//Calculated as log2(area)
	static constexpr size_t areaShift = 16;

	/**
	 * Creates a map for representing the filled areas in a chunk.
	 */
	BlockMap() {}

	/**
	 * Sets the given postion as filled or empty in the map.
	 * @param x,y,z The position to set.
	 * @param val Whether to set the block as filled or empty.
	 */
	void setBlockFill(size_t x, size_t y, size_t z, bool val = true) {
		map[x * area + y * length + z] = val;
	}

	/**
	 * Gets whether the block at the given position is filled.
	 * @param x,y,z The position to check.
	 * @return Whether the given block was filled.
	 */
	bool isBlockFilled(size_t x, size_t y, size_t z) const {
		return map[x * area + y * length + z];
	}

	/**
	 * Sets the area encompassed by the region as filled.
	 * @param region The region to set as filled.
	 */
	void addRegionFill(const InternalRegion& region) {
		Aabb<uint64_t> regBox(region.box);
		regBox.max += Aabb<uint64_t>::vec_t(1, 1, 1);

		Aabb<uint64_t>::vec_t min = regBox.min;
		Aabb<uint64_t>::vec_t max = regBox.max;

		//It doesn't matter if max underflows, it'll just end up as 0, as it should
		uint64_t mask1 = regionMask(min.z, max.z, 0, 64);
		uint64_t mask2 = regionMask(min.z, max.z, 64, 128);
		uint64_t mask3 = regionMask(min.z, max.z, 128, 192);
		uint64_t mask4 = regionMask(min.z, max.z, 192, 256);

		for (uint64_t x = min.x; x < max.x; x++) {
			for (uint64_t y = min.y; y < max.y; y++) {
				//Hack to get internal data. Need own implementation later to avoid undefined behaviour.
				uint64_t* mapData = (uint64_t*)&map;

				uint64_t baseIndex = (x * area + y * length) / 64;
				mapData[baseIndex] |= mask1;
				mapData[baseIndex + 1] |= mask2;
				mapData[baseIndex + 2] |= mask3;
				mapData[baseIndex + 3] |= mask4;
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
		if (fixed == 0 || fixed == length) {
			return true;
		}

		//Innermost loop along z axis, allows for fused visibility checks
		//This is for the east, west, up, and down axis
		if (normal != 0 && normal != 2) {
			uint64_t minZ = face.min.at(1);
			uint64_t maxZ = face.max.at(1);

			if (normal == 3 || normal == 5) {
				fixed--;
			}

			uint64_t shift1 = areaShift;
			uint64_t shift2 = lengthShift;

			if (normal == 4 || normal == 5) {
				std::swap(shift1, shift2);
			}

			uint64_t mask1 = regionMask(minZ, maxZ, 0, 64);
			uint64_t mask2 = regionMask(minZ, maxZ, 64, 128);
			uint64_t mask3 = regionMask(minZ, maxZ, 128, 192);
			uint64_t mask4 = regionMask(minZ, maxZ, 192, 256);

			//i is either y or x, depending on the normal direction - east/west
			//is y, up/down is x
			for (uint64_t i = face.min.at(0); i < face.max.at(0); i++) {
				uint64_t* mapData = (uint64_t*)&map;

				uint64_t baseIndex = ((fixed << shift1) + (i << shift2)) / 64;
				visible =
					((mask1 & mapData[baseIndex]) | ~mask1) != 0xFFFFFFFFFFFFFFFF ||
					((mask2 & mapData[baseIndex + 1]) | ~mask2) != 0xFFFFFFFFFFFFFFFF ||
					((mask3 & mapData[baseIndex + 2]) | ~mask3) != 0xFFFFFFFFFFFFFFFF ||
					((mask4 & mapData[baseIndex + 3]) | ~mask4) != 0xFFFFFFFFFFFFFFFF;

				if (visible) {
					return visible;
				}
			}

			return false;
		}

		for (uint64_t i = face.min.at(0); i < face.max.at(0); i++) {
			for (uint64_t j = face.min.at(1); j < face.max.at(1); j++) {
				if (normal == 0) {
					visible = !isBlockFilled(i, j, fixed - 1);
				}
				else if (normal == 2) {
					visible = !isBlockFilled(i, j, fixed);
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
	const std::bitset<volume>& getMap() const { return map; }

private:
	//Stores which blocks are filled in the chunk.
	std::bitset<volume> map;

	static constexpr uint64_t regionMask(uint64_t min, uint64_t max, uint64_t clampMin, uint64_t clampMax) {
		uint64_t shiftMin = std::max(min, clampMin) - clampMin;
		uint64_t shiftMax = clampMax - std::min(max, clampMax);

		//As it turns out, shifting past bitwidth is undefined behaviour.
		if (shiftMin >= 64 || shiftMax >= 64) {
			return 0;
		}

		return (0xFFFFFFFFFFFFFFFFul << shiftMin) & (0xFFFFFFFFFFFFFFFFul >> shiftMax);
	}
};
