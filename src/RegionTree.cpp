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
#include "BlockMap.hpp"

void RegionTree::addRegions(std::vector<InternalRegion> addRegs) {
	children.clear();
	regions.clear();

	if (addRegs.size() <= splitCount) {
		regions = addRegs;
		return;
	}

	//Find optimal splits for this node
	glm::vec3 avgCenter(0.0f, 0.0f, 0.0f);

	for (const InternalRegion& reg : addRegs) {
		avgCenter += Aabb<float>(reg.box).getCenter();
	}

	avgCenter /= addRegs.size();
	std::array<std::array<size_t, 3>, 3> boxSplitCounts = {};

	//Determine best axis for split (least variation between halves)
	for (const InternalRegion& reg : addRegs) {
		for(size_t axis = 0; axis < 3; axis++) {
			if (reg.box.max[axis] < avgCenter[axis]) {
				boxSplitCounts[axis][0]++;
			}
			else if (reg.box.min[axis] > avgCenter[axis]) {
				boxSplitCounts[axis][2]++;
			}
			else {
				boxSplitCounts[axis][1]++;
			}
		}
	}

	std::array<size_t, 3> axisScores = {};

	for (size_t i = 0; i < 3; i++) {
		size_t left = boxSplitCounts[i][0];
		size_t center = boxSplitCounts[i][1];
		size_t right = boxSplitCounts[i][2];

		size_t lrDiff = std::max(left, right) - std::min(left, right);
		size_t lcDiff = std::max(left, center) - std::min(left, center);
		size_t rcDiff = std::max(right, center) - std::min(right, center);

		axisScores[i] = lrDiff + lcDiff + rcDiff;
	}

	size_t minAxis = 0;

	for (size_t i = 0; i < 3; i++) {
		minAxis = axisScores[i] < axisScores[minAxis] ? i : minAxis;
	}

	size_t splitBlock = (uint8_t) avgCenter[minAxis];

	//Force extra regions to move to children
	if (boxSplitCounts[minAxis][1] > splitCount) {
		size_t toRemove = boxSplitCounts[minAxis][1] - splitCount;

		for (size_t i = 0; i < addRegs.size(); i++) {
			InternalRegion& reg = addRegs.at(i);

			if (reg.box.min[minAxis] <= splitBlock && reg.box.max[minAxis] > splitBlock) {
				std::array<Aabb<uint8_t>, 2> boxes = reg.box.bisect(minAxis, splitBlock);
				boxes[1].min[minAxis]++;
				reg.box = boxes[0];

				if (boxes[1].min[minAxis] <= boxes[1].max[minAxis]) {
					addRegs.push_back({reg.type, boxes[1]});
				}

				toRemove--;

				if (toRemove == 0) {
					break;
				}
			}
		}
	}

	//Create children
	std::array<Aabb<uint8_t>, 2> childBoxes = box.bisect(minAxis, splitBlock);

	for (Aabb<uint8_t> childBox : childBoxes) {
		//Internal regions are stored as blocks, not bounding boxes, so
		//we have to make sure two children can't contain the same region
		if (childBox.min[minAxis] == splitBlock) childBox.min[minAxis]++;

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
	BlockMap map(box.min, Aabb<uint64_t>::vec_t(box.max) + Aabb<uint64_t>::vec_t(1, 1, 1));

	fillMap(map);

	std::vector<RegionFace> faces;
	generateFaces(map, faces);

	return faces;
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

void RegionTree::fillMap(BlockMap& map) const {
	for (const InternalRegion& reg : regions) {
		map.addRegionFill(reg);
	}

	for (const RegionTree& child : children) {
		child.fillMap(map);
	}
}

void RegionTree::generateFaces(const BlockMap& map, std::vector<RegionFace>& faces) const {
	//Collect all region faces, discard those which are covered
	for (const InternalRegion& region : regions) {
		std::array<RegionFace, 6> genFaces = genRegionFaces(region);

		for (const RegionFace& face : genFaces) {
			if (map.isFaceVisible(face)) {
				faces.push_back(face);
			}
		}
	}

	//Recurse to children
	for (const RegionTree& child : children) {
		child.generateFaces(map, faces);
	}
}

std::array<RegionFace, 6> RegionTree::genRegionFaces(const InternalRegion& region) const {
	//Don't bother to use the expanded box here, it would only require more casting
	Aabb<uint16_t>::vec_t min = region.box.min;
	Aabb<uint16_t>::vec_t max(region.box.max.x + 1, region.box.max.y + 1, region.box.max.z + 1);

	return {
		//north
		RegionFace{min.z, {min.x, min.y}, {max.x, max.y}, region.type},
		//east
		RegionFace{(uint16_t) (0x200 | max.x), {min.y, min.z}, {max.y, max.z}, region.type},
		//south
		RegionFace{(uint16_t) (0x400 | max.z), {min.x, min.y}, {max.x, max.y}, region.type},
		//west
		RegionFace{(uint16_t) (0x600 | min.x), {min.y, min.z}, {max.y, max.z}, region.type},
		//up
		RegionFace{(uint16_t) (0x800 | max.y), {min.x, min.z}, {max.x, max.z}, region.type},
		//down
		RegionFace{(uint16_t) (0xA00 | min.y), {min.x, min.z}, {max.x, max.z}, region.type}
	};
}
