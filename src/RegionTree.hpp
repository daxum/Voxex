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
	uint8_t fixedCoord;
	//Stores the minimum and maximum coordinates
	//in the two used axis.
	std::array<uint8_t, 2> min;
	std::array<uint8_t, 2> max;
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

	RegionTree(size_t splitCount, Aabb<uint8_t> box = Aabb<uint8_t>({0, 0, 0}, {255, 255, 255}));

	void addRegions(std::vector<InternalRegion>& addRegs);

	size_t size() const ;

	const std::vector<RegionTree>& getChildren() const { return children; }

	const std::vector<InternalRegion>& getRegions() const { return regions; }

	std::vector<RegionFace> genQuads() const;

	/**
	 * First is "outer" faces, which might intersect with other children.
	 * Second is "inner" faces, which will only intersect with the parent.
	 */
	std::pair<FaceList, FaceList> genQuadsInternal() const;

	void printCounts(std::string idents = "") const;

	size_t getNodeCount() const;

	size_t getMemUsage() const;

	void optimizeTree();

	bool isLeaf() const { return children.empty(); }

private:
	Aabb<uint8_t> box;
	size_t splitCount;
	std::vector<RegionTree> children;
	std::vector<InternalRegion> regions;

	void trySplitTree();

	static void deduplicateFaces(FaceList& faceList);

	static void deduplicateAdd(FaceList& addList, RegionFace face);

	static bool isOnEdge(const RegionFace& face, const Aabb<uint8_t>& box) {
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

	static void insertFaceList(FaceList& list, const FaceList& insert) {
		for (size_t i = 0; i < list.size(); i++) {
			list.at(i).insert(list.at(i).end(), insert.at(i).begin(), insert.at(i).end());
		}
	}

	static void addFaceToList(FaceList& list, const RegionFace& face) {
		list.at(face.getUnusedAxis()).push_back(face);
	}

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

	static void handleFaceIntersection(RegionFace& face1, RegionFace& face2);

	static size_t faceListSize(const FaceList& list) {
		size_t count = 0;

		for (const auto& vec : list) {
			count += vec.size();
		}

		return count;
	}
};
