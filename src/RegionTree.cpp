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

#include "RegionTree.hpp"

void RegionTree::addRegions(std::vector<InternalRegion> addRegs) {
	children.clear();
	regions.clear();

	if (addRegs.size() < splitCount) {
		regions = addRegs;
		return;
	}

	//Find optimal splits for this node
	glm::vec3 avgCenter(0.0f, 0.0f, 0.0f);

	for (const InternalRegion& reg : addRegs) {
		avgCenter += Aabb<float>(reg.box).getCenter();
	}

	avgCenter /= addRegs.size();
	Aabb<uint8_t>::vec_t centerSplit(avgCenter);

	//Create children
	std::array<Aabb<uint8_t>, 8> childBoxes = box.split(centerSplit);

	for (Aabb<uint8_t> childBox : childBoxes) {
		//Internal regions are stored as blocks, not bounding boxes, so
		//we have to make sure two children can't contain the same region
		if (childBox.min.x != 0) childBox.min.x++;
		if (childBox.min.y != 0) childBox.min.y++;
		if (childBox.min.z != 0) childBox.min.z++;

		children.emplace_back(childBox);

		//Add regions to child
		std::vector<InternalRegion> childAdd;

		for (size_t i = addRegs.size() - 1; i + 1 > 0; i--) {
			InternalRegion reg = addRegs.at(i);

			if (childBox.contains(reg.box)) {
				childAdd.push_back(reg);
				addRegs.at(i) = addRegs.back();
				addRegs.pop_back();
			}
		}

		children.back().addRegions(childAdd);
	}

	//Everything else goes in this node
	regions = addRegs;
}

size_t RegionTree::size() const {
	size_t count = regions.size();

	for (const RegionTree& child : children) {
		count += child.size();
	}

	return count;
}

std::vector<RegionFace> RegionTree::genQuads() const {
	double start = ExMath::getTimeMillis();

	auto vecs = genQuadsInternal();
	std::vector<RegionFace> faces = flattenList(vecs);

	double end = ExMath::getTimeMillis();

	std::cout << "Reduced from " << (size() * 6) << " to " << faces.size() << " faces - " <<
				 "completed in " << (end-start) << "ms\n";

	return faces;
}

std::pair<RegionTree::FaceList, RegionTree::FaceList> RegionTree::genQuadsInternal() const {
	FaceList outerFaces;
	FaceList innerFaces;

	for (const RegionTree& child : children) {
		std::pair<FaceList, FaceList> childFaces = child.genQuadsInternal();
		insertFaceList(outerFaces, childFaces.first);
		insertFaceList(innerFaces, childFaces.second);
	}

	deduplicateFaces(outerFaces);

	//Expand the node's box from a block range to a bounding box
	Aabb<uint16_t> expandedBox = box;
	expandedBox.max += Aabb<uint16_t>::vec_t(1, 1, 1);

	//Move inner faces for this box out of outer face list
	for (std::vector<RegionFace>& outerList : outerFaces) {
		for (size_t i = outerList.size() - 1; i + 1 > 0; i--) {
			if (!isOnEdge(outerList.at(i), expandedBox)) {
				addFaceToList(innerFaces, outerList.at(i));
				outerList.at(i) = outerList.back();
				outerList.pop_back();
			}
		}
	}

	FaceList currentFaces;

	//Collect all region faces
	for (const InternalRegion& region : regions) {
		//Expand region box similarly to the node box above
		Aabb<uint32_t> fullRegionBox = region.box;
		fullRegionBox.max += Aabb<uint32_t>::vec_t(1, 1, 1);

		uint32_t xArea = fullRegionBox.yLength() * fullRegionBox.zLength();
		uint32_t yArea = fullRegionBox.xLength() * fullRegionBox.zLength();
		uint32_t zArea = fullRegionBox.xLength() * fullRegionBox.yLength();

		//Don't bother to use the expanded box here, it would only require more casting
		Aabb<uint16_t>::vec_t min = region.box.min;
		Aabb<uint16_t>::vec_t max(region.box.max.x + 1, region.box.max.y + 1, region.box.max.z + 1);

		//north
		addFaceToList(currentFaces, {0x80, min.z, {min.x, min.y}, {max.x, max.y}, region.type, 0, zArea});
		//east
		addFaceToList(currentFaces, {0x01, min.x, {min.y, min.z}, {max.y, max.z}, region.type, 0, xArea});
		//south
		addFaceToList(currentFaces, {0x82, max.z, {min.x, min.y}, {max.x, max.y}, region.type, 0, zArea});
		//west
		addFaceToList(currentFaces, {0x03, max.x, {min.y, min.z}, {max.y, max.z}, region.type, 0, xArea});
		//up
		addFaceToList(currentFaces, {0x44, max.y, {min.x, min.z}, {max.x, max.z}, region.type, 0, yArea});
		//down
		addFaceToList(currentFaces, {0x45, min.y, {min.x, min.z}, {max.x, max.z}, region.type, 0, yArea});
	}

	//Sort into inner and outer
	for (size_t i = 0; i < currentFaces.size(); i++) {
		for (const RegionFace& face : currentFaces.at(i)) {
			if (isOnEdge(face, expandedBox)) {
				deduplicateAdd(outerFaces, face);
			}
			else {
				deduplicateAdd(innerFaces, face);
			}
		}
	}

	return {outerFaces, innerFaces};
}

void RegionTree::printCounts(std::string idents) const {
	std::cout << idents << regions.size() << "\n";
	for (const RegionTree& child : children) {
		child.printCounts(idents + "  ");
	}
}

size_t RegionTree::getNodeCount() const {
	size_t count = 1;

	for (const RegionTree& child : children) {
		count += child.getNodeCount();
	}

	return count;
}

size_t RegionTree::getMemUsage() const {
	size_t mem = sizeof(RegionTree);

	mem += regions.capacity() * sizeof(InternalRegion);

	for (const RegionTree& child : children) {
		mem += child.getMemUsage() - sizeof(RegionTree);
	}

	mem += children.capacity() * sizeof(RegionTree);

	return mem;
}

void RegionTree::optimizeTree() {
	/*regions.reserve(size());

	for (RegionTree& child : children) {
		child.optimizeTree();
		regions.insert(regions.end(), child.regions.begin(), child.regions.end());
		child.regions.clear();
	}

	for (size_t i = 0; i < regions.size(); i++) {
		for (size_t j = i + 1; j < regions.size(); j++) {
			if (regions.at(i).box.formsBoxWith(regions.at(j).box) && regions.at(i).type == regions.at(j).type) {
				regions.at(i) = InternalRegion{regions.at(i).type, Aabb<uint8_t>(regions.at(i).box, regions.at(j).box)};
				regions.at(j) = regions.back();
				regions.pop_back();
				i = 0 - 1;
				break;
			}
		}
	}

	bool pruneChildren = true;

	for (RegionTree& child : children) {
		child.addRegions(regions);

		if (!child.isLeaf() || !child.regions.empty()) {
			pruneChildren = false;
		}
	}

	if (!isLeaf() && pruneChildren) {
		children.clear();
	}

	regions.shrink_to_fit();*/
}

void RegionTree::trySplitTree() {
	//Don't split more than once, and don't go beyond min length
	if (children.size() == 0 && box.xLength() > 1) {
		std::array<Aabb<uint8_t>, 8> childBoxes = box.split();

		for (Aabb<uint8_t> childBox : childBoxes) {
			//Internal regions are stored as blocks, not bounding boxes, so
			//we have to make sure two children can't contain the same region
			if (childBox.min.x != 0) childBox.min.x++;
			if (childBox.min.y != 0) childBox.min.y++;
			if (childBox.min.z != 0) childBox.min.z++;

			children.emplace_back(childBox);
		}

		//Try to distribute boxes among children
		for (RegionTree& child : children) {
			child.addRegions(regions);
		}
	}
}

void RegionTree::deduplicateFaces(FaceList& faceList) {
	for (std::vector<RegionFace>& faces : faceList) {
		for (size_t i = 0; i < faces.size(); i++) {
			//Skip faces already covered to shave off a few more milliseconds.
			//Causes a very small number of addition faces to be missed (~50/270000)
			if (faces.at(i).fullyCovered()) {
				continue;
			}

			for (size_t j = i + 1; j < faces.size(); j++) {
				RegionFace& face1 = faces.at(i);
				RegionFace& face2 = faces.at(j);

				handleFaceIntersection(face1, face2);
			}
		}

		//Iterate through and remove all faces where coveredArea >= totalArea (completely covered)
		for (size_t i = 0; i < faces.size(); i++) {
			if (faces.at(i).fullyCovered()) {
				faces.at(i) = faces.back();
				faces.pop_back();
				i--;
			}
		}
	}
}

void RegionTree::deduplicateAdd(FaceList& addList, RegionFace face) {
	std::vector<RegionFace>& list = addList.at(face.getUnusedAxis());

	for (size_t i = list.size() - 1; i + 1 > 0; i--) {
		handleFaceIntersection(face, list.at(i));

		if (list.at(i).fullyCovered()) {
			list.at(i) = list.back();
			list.pop_back();
		}

		if (face.fullyCovered()) {
			return;
		}
	}

	list.push_back(face);
}

void RegionTree::handleFaceIntersection(RegionFace& face1, RegionFace& face2) {
	if (face1.min.at(0) < face2.max.at(0) && face1.max.at(0) > face2.min.at(0) &&
		face1.min.at(1) < face2.max.at(1) && face1.max.at(1) > face2.min.at(1) &&
		face1.fixedCoord == face2.fixedCoord) {
		//Find intersecting area
		uint32_t interMinX = std::max(face1.min.at(0), face2.min.at(0));
		uint32_t interMinY = std::max(face1.min.at(1), face2.min.at(1));
		uint32_t interMaxX = std::min(face1.max.at(0), face2.max.at(0));
		uint32_t interMaxY = std::min(face1.max.at(1), face2.max.at(1));

		uint32_t interArea = (interMaxX - interMinX) * (interMaxY - interMinY);

		//Add to covered area
		face1.coveredArea += interArea;
		face2.coveredArea += interArea;
	}
}
