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

RegionTree::RegionTree(size_t splitCount, Aabb<uint8_t> box) :
	box(box),
	splitCount(splitCount) {

}

void RegionTree::addRegions(std::vector<InternalRegion>& addRegs) {
	//Split up regions to remove intersections with this node
	for (size_t i = 0; i < addRegs.size(); i++) {
		InternalRegion& next = addRegs.at(i);

		for (const InternalRegion& region : regions) {
			//Split node if intersecting
			if (next.box.intersects(region.box)) {
				std::vector<Aabb<uint8_t>> subRegions = next.box.subtract(region.box);

				for (const Aabb<uint8_t>& box : subRegions) {
					addRegs.push_back(InternalRegion{region.type, box});
				}

				//Remove split box
				addRegs.at(i) = addRegs.back();
				addRegs.pop_back();
				i--;

				break;
			}
		}
	}

	//Recurse to children, so the smallest boxes end up at the bottom of the tree,
	//and all boxes get split (no intersections!)
	for (RegionTree& child : children) {
		for (const InternalRegion& reg : addRegs) {
			//Only recurse if we have a box that intersects with the child
			if (reg.box.intersects(child.box)) {
				child.addRegions(addRegs);
				break;
			}
		}
	}

	//Add whatever's left that fits in this box
	for (size_t i = 0; i < addRegs.size(); i++) {
		InternalRegion& reg = addRegs.at(i);

		if (box.contains(reg.box)) {
			regions.push_back(reg);
			addRegs.at(i) = addRegs.back();
			addRegs.pop_back();
			i--;
		}
	}

	//Split tree if overloaded
	if (regions.size() > splitCount) {
		trySplitTree();
	}
}

size_t RegionTree::size() const {
	size_t count = regions.size();

	for (const RegionTree& child : children) {
		count += child.size();
	}

	return count;
}

std::vector<RegionFace> RegionTree::genQuads() const {
	std::cout << "Originally at " << (size() * 6) << " faces!\n";

	double start = ExMath::getTimeMillis();

	auto vecs = genQuadsInternal();
	std::vector<RegionFace> faces = flattenList(vecs);

	double end = ExMath::getTimeMillis();

	std::cout << "Reduced to " << faces.size() << " faces!\n";
	std::cout << "Generate faces completed in " << (end-start) << "ms\n";

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

	//Move inner faces for this box out of outer face list
	for (size_t i = 0; i < outerFaces.size(); i++) {
		std::vector<RegionFace>& outerList = outerFaces.at(i);

		for (size_t j = 0; j < outerList.size(); j++) {
			if (!isOnEdge(outerList.at(j), box)) {
				addFaceToList(innerFaces, outerList.at(j));
				outerList.at(j) = outerList.back();
				outerList.pop_back();
				j--;
			}
		}
	}

	FaceList currentFaces;

	//Collect all region faces
	for (const InternalRegion& region : regions) {
		uint32_t xArea = (uint32_t)region.box.yLength() * (uint32_t)region.box.zLength();
		uint32_t yArea = (uint32_t)region.box.xLength() * (uint32_t)region.box.zLength();
		uint32_t zArea = (uint32_t)region.box.xLength() * (uint32_t)region.box.yLength();

		//north
		addFaceToList(currentFaces, {0x80, region.box.min.z, {region.box.min.x, region.box.min.y}, {region.box.max.x, region.box.max.y}, region.type, 0, zArea});
		//east
		addFaceToList(currentFaces, {0x01, region.box.min.x, {region.box.min.y, region.box.min.z}, {region.box.max.y, region.box.max.z}, region.type, 0, xArea});
		//south
		addFaceToList(currentFaces, {0x82, region.box.max.z, {region.box.min.x, region.box.min.y}, {region.box.max.x, region.box.max.y}, region.type, 0, zArea});
		//west
		addFaceToList(currentFaces, {0x03, region.box.max.x, {region.box.min.y, region.box.min.z}, {region.box.max.y, region.box.max.z}, region.type, 0, xArea});
		//up
		addFaceToList(currentFaces, {0x44, region.box.max.y, {region.box.min.x, region.box.min.z}, {region.box.max.x, region.box.max.z}, region.type, 0, yArea});
		//down
		addFaceToList(currentFaces, {0x45, region.box.min.y, {region.box.min.x, region.box.min.z}, {region.box.max.x, region.box.max.z}, region.type, 0, yArea});
	}

	//Sort into inner and outer
	for (size_t i = 0; i < currentFaces.size(); i++) {
		for (const RegionFace& face : currentFaces.at(i)) {
			if (isOnEdge(face, box)) {
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
	regions.reserve(size());

	for (RegionTree& child : children) {
		child.optimizeTree();
		regions.insert(regions.end(), child.regions.begin(), child.regions.end());
		child.regions.clear();
	}

	for (size_t i = 0; i < regions.size(); i++) {
		for (size_t j = i + 1; j < regions.size(); j++) {
			if (regions.at(i).box.formsBoxWith(regions.at(j).box) /*&& regions.at(i).type == regions.at(j).type*/) {
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

	regions.shrink_to_fit();
}

void RegionTree::trySplitTree() {
	//Don't split more than once, and don't go beyond min length
	if (children.size() == 0 && box.xLength() > 1) {
		std::array<Aabb<uint8_t>, 8> childBoxes = box.split();

		for (Aabb<uint8_t> childBox : childBoxes) {
			children.emplace_back(splitCount, childBox);
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
			if (faces.at(i).coveredArea >= faces.at(i).totalArea) {
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

	for (size_t i = 0; i < list.size(); i++) {
		handleFaceIntersection(face, list.at(i));

		if (list.at(i).fullyCovered()) {
			list.at(i) = list.back();
			list.pop_back();
			i--;
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
