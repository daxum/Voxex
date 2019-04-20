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
#include <queue>

#include "AxisAlignedBB.hpp"

class BlockMap;

struct InternalRegion {
	uint16_t type;
	Aabb<uint8_t> box;
};

struct RegionFace {
	//Stores the normal data - 0:north, 1:east, 2:south, 3:west, 4:up, 5:down.
	//Upper 2 bits store the unused axis. Don't use directly, see functions
	//below.
	uint8_t normalAxis;
	//Stores the coordinate for the third, fixed, axis.
	uint16_t fixedCoord;
	//Stores the minimum and maximum coordinates
	//in the two used axis.
	std::array<uint16_t, 2> min;
	std::array<uint16_t, 2> max;
	//Stores the type of region this face is for.
	uint16_t type;
	//Area covered by other faces.
	uint32_t coveredArea;
	//Total area - only really present for alignment.
	uint32_t totalArea;

	uint8_t getNormal() const { return normalAxis & 0x3F; }
	uint8_t getUnusedAxis() const { return (normalAxis & 0xC0) >> 6; }
	bool fullyCovered() const { return coveredArea >= totalArea; }
};

inline std::ostream& operator<<(std::ostream& out, const RegionFace& face) {
	out << "Face[";

	switch (face.getNormal()) {
		//Z
		case 0:
		case 2:
			out << face.min.at(0) << ", " << face.min.at(1) << ", " << face.fixedCoord << " | " << face.max.at(0) << ", " << face.max.at(1) << ", " << face.fixedCoord;
			break;
		//X
		case 1:
		case 3:
			out << face.fixedCoord << ", " << face.min.at(0) << ", " << face.min.at(1) << " | " << face.fixedCoord << ", " << face.max.at(0) << ", " << face.max.at(1);
			break;
		//Y
		case 4:
		case 5:
			out << face.min.at(0) << ", " << face.fixedCoord << ", " << face.min.at(1) << " | " << face.max.at(0) << ", " << face.fixedCoord << ", " << face.max.at(1);
			break;
		default: out << "Invalid position!";
	}

	out << " | facing: " << (uint64_t)face.getNormal() << "]";

	return out;
}

class RegionTree {
public:
	constexpr static size_t splitCount = 1024;

	/**
	 * Constructs an empry tree with the given bounding box.
	 * @param box The box for the node.
	 */
	RegionTree(Aabb<uint8_t> box = Aabb<uint8_t>({0, 0, 0}, {255, 255, 255})) : box(box) {}

	/**
	 * Adds a list of regions to the tree. This does not do any checking for
	 * if the regions are intersecting or such, and is primarily intended to
	 * be called when generating a chunk or loading from disk. When this is
	 * called, it is assumed the the node's internal region and child
	 * lists are empty - if they are not, all existing regions and children
	 * are discarded.
	 * @param addRegs The regions to add.
	 */
	void addRegions(std::vector<InternalRegion> addRegs);

	/**
	 * Gets the number of regions stored in the tree.
	 * @return The number of regions.
	 */
	size_t size() const ;

	/**
	 * Generates faces from the stored regions so the chunk can be rendered.
	 * @return A list of faces making up the mesh.
	 */
	std::vector<RegionFace> genQuads() const;

	/**
	 * Prints the number of regions for each node in a nicely formatted manner.
	 * @param The number of indentations to put before the count.
	 */
	void printCounts(std::string idents = "") const;

	/**
	 * Counts the number of nodes in the tree.
	 * @return The number of nodes in the tree.
	 */
	size_t getNodeCount() const;

	/**
	 * Calculates the amount of memory used by the tree, its nodes, and the
	 * regions stored in the nodes.
	 * @return The total memory usage for the tree, in bytes.
	 */
	size_t getMemUsage() const;

	/**
	 * Returns whether this node is a leaf node. Leaf nodes
	 * have no children.
	 * @return Whether this node is a leaf.
	 */
	bool isLeaf() const { return children.empty(); }

private:
	//Bounding box for this node. This is a block range, not a containing volume.
	Aabb<uint8_t> box;
	//Child nodes of this node.
	std::vector<RegionTree> children;
	//All regions stored in this node.
	std::vector<InternalRegion> regions;

	/**
	 * Checks if the tree is overloaded, and if it is, creates children and distributes
	 * its regions among them.
	 */
	void trySplitTree();

	/**
	 * Fills the given block map with the regions stored in this tree.
	 * @param map The map to fill.
	 */
	void fillMap(BlockMap& map) const;

	/**
	 * Generates faces for this tree's regions. All the faces are external and
	 * fully uncovered, provided the map is correct.
	 * @param map The map to use for coverage lookups.
	 * @param faces A vector to store the generated faces in.
	 */
	void generateFaces(const BlockMap& map, std::vector<RegionFace>& faces) const;

	/**
	 * Generates the faces for the provided region.
	 * @param reg The region to generate faces for.
	 * @return The generated faces.
	 */
	std::array<RegionFace, 6> genRegionFaces(const InternalRegion& region) const;
};
