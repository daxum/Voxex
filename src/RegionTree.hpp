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
	out << "Face[" << (uint64_t)face.min.at(0) << ", " << (uint64_t)face.min.at(1) << " | " <<
		(uint64_t)face.max.at(0) << ", " << (uint64_t)face.max.at(1) <<
		" | Unused: " << (uint64_t)face.getUnusedAxis() << ", fixed: " << (uint64_t)face.fixedCoord << "]";

	return out;
}

class RegionTree {
public:
	typedef std::array<std::vector<RegionFace>, 3> FaceList;

	constexpr static size_t splitCount = 512;

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
	 * Attempts to lower the total region count by merging adjacent regions
	 * of the same type.
	 */
	void optimizeTree();

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
	 * Generates faces for the regions stored in this node and all its children.
	 * @return A pair of lists of face. The first element is the "outer" faces,
	 *     which might intersect with the other children of the parent node, and the
	 *     second element is "inner" faces, which will only intersect with the parent.
	 */
	std::pair<FaceList, FaceList> genQuadsInternal() const;

	/**
	 * Removes any faces from the given list which are fully covered by other faces.
	 * @param faceList The list to deduplicate. It should be as small as possible for
	 *     good performance.
	 */
	static void deduplicateFaces(FaceList& faceList);

	/**
	 * Checks the face against every face in the given list, removing any faces that
	 * would become fully covered by the given face. If the provided face is not fully
	 * covered after every face is checked, it is added to the list.
	 * @param addList The list to add the face to.
	 * @param face The face to add.
	 */
	static void deduplicateAdd(FaceList& addList, RegionFace face);

	/**
	 * Returns whether the face is on the edge of the given box.
	 * @param face The face to check.
	 * @param box The box to check against.
	 * @return Whether the face intersects with any face of the provided box.
	 */
	static bool isOnEdge(const RegionFace& face, const Aabb<uint16_t>& box) {
		switch (face.getUnusedAxis()) {
			//X
			case 0: return face.fixedCoord == box.min.x || face.fixedCoord == box.max.x;
			//Y
			case 1: return face.fixedCoord == box.min.y || face.fixedCoord == box.max.y;
			//Z
			case 2: return face.fixedCoord == box.min.z || face.fixedCoord == box.max.z;
			default: throw std::runtime_error("Recieved a >3 dimensional square");
		}
	}

	/**
	 * Merges two face lists.
	 * @param list The list to insert into.
	 * @param instert The list to insert.
	 */
	static void insertFaceList(FaceList& list, const FaceList& insert) {
		for (size_t i = 0; i < list.size(); i++) {
			list.at(i).insert(list.at(i).end(), insert.at(i).begin(), insert.at(i).end());
		}
	}

	/**
	 * Directly adds a face to a facelist.
	 * @param list The list to add to.
	 * @param face The face to add.
	 */
	static void addFaceToList(FaceList& list, const RegionFace& face) {
		list.at(face.getUnusedAxis()).push_back(face);
	}

	/**
	 * Merges the lists contained in the two FaceLists into a single list of all faces.
	 * @param list The two lists two flatten.
	 * @return The flattened list.
	 */
	static std::vector<RegionFace> flattenList(const std::pair<FaceList, FaceList>& list) {
		std::vector<RegionFace> faces;

		for (const std::vector<RegionFace>& faceVec : list.first) {
			faces.insert(faces.end(), faceVec.begin(), faceVec.end());
		}

		for (const std::vector<RegionFace>& faceVec : list.second) {
			faces.insert(faces.end(), faceVec.begin(), faceVec.end());
		}

		return faces;
	}

	/**
	 * Checks whether the two faces are intersecting, and if they are, adds the
	 * intersecting area to their total areas.
	 * @param face1 The first face to check.
	 * @param face2 The second face to check.
	 */
	static void handleFaceIntersection(RegionFace& face1, RegionFace& face2);

	/**
	 * Returns the size of the face list.
	 * @param list The list to get the size of.
	 * @return The size of the list.
	 */
	static size_t faceListSize(const FaceList& list) {
		size_t count = 0;

		for (const auto& vec : list) {
			count += vec.size();
		}

		return count;
	}
};
