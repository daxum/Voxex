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

#include "Chunk.hpp"

namespace {
	/**
	 * Offset should be multiple of 256 (length of chunk).
	 */
	/**Region fromInternal(const InternalRegion& internal, const Pos_t& offset) :
		type(internal.type),
		box(internal.box) {

		box.min += offset;
		box.max += offset;
	}**/

	uint8_t reduceCoords(int64_t val) {
		return (val < 0) ? 256 - (((-val) & 255)) : val & 255;
	}

	InternalRegion toInternal(const Region& region) {
		InternalRegion reg = {};
		reg.type = region.type;

		reg.box.min.x = reduceCoords(region.box.min.x);
		reg.box.min.y = reduceCoords(region.box.min.y);
		reg.box.min.z = reduceCoords(region.box.min.z);

		reg.box.max.x = reduceCoords(region.box.max.x);
		reg.box.max.y = reduceCoords(region.box.max.y);
		reg.box.max.z = reduceCoords(region.box.max.z);

		return reg;
	}
}

void Chunk::addRegion(const Region& reg) {
	if (!box.contains(reg.box)) {
		std::cout << reg.box << "\n";
		throw std::invalid_argument("Attempt to add region not within chunk!");
	}

	if (reg.box.getVolume() == 0) {
		std::cout << reg.box << "\n";
		throw std::runtime_error("Attempted to add 0-volume box!");
	}

	InternalRegion region = toInternal(reg);
	region.box.validate();

	std::vector<InternalRegion> addVec = {region};
	regions.addRegions(addVec);
}

std::pair<std::vector<ChunkVertex>, std::vector<uint32_t>> Chunk::generateMesh(const std::array<std::array<float, 3>, 20>& colors) {
	std::vector<RegionFace> faces = regions.genQuads();

	std::vector<ChunkVertex> vertices;
	std::vector<uint32_t> indices;

	for (const RegionFace& face : faces) {
		size_t baseIndex = vertices.size();

		switch (face.getNormal()) {
			//Facing -z
			case 0: {
				glm::vec3 normal(0.0, 0.0, -1.0);
				vertices.push_back(ChunkVertex{{face.min.at(0), face.min.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.min.at(0), face.max.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.min.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.max.at(1), face.fixedCoord}, normal, colors.at(face.type)});
			} break;
			//Facing -x
			case 1: {
				glm::vec3 normal(-1.0, 0.0, 0.0);
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.min.at(0), face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.max.at(0), face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.min.at(0), face.min.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.max.at(0), face.min.at(1)}, normal, colors.at(face.type)});
			} break;
			//Facing +z
			case 2: {
				glm::vec3 normal(0.0, 0.0, 1.0);
				vertices.push_back(ChunkVertex{{face.max.at(0), face.min.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.max.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.min.at(0), face.min.at(1), face.fixedCoord}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.min.at(0), face.max.at(1), face.fixedCoord}, normal, colors.at(face.type)});
			} break;
			//Facing +x
			case 3: {
				glm::vec3 normal(1.0, 0.0, 0.0);
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.max.at(0), face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.min.at(0), face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.max.at(0), face.min.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.fixedCoord, face.min.at(0), face.min.at(1)}, normal, colors.at(face.type)});
			} break;
			//Facing +y
			case 4: {
				glm::vec3 normal(0.0, 1.0, 0.0);
				vertices.push_back(ChunkVertex{{face.min.at(0), face.fixedCoord, face.min.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.min.at(0), face.fixedCoord, face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.fixedCoord, face.min.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.fixedCoord, face.max.at(1)}, normal, colors.at(face.type)});
			} break;
			//Facing -y
			case 5: {
				glm::vec3 normal(0.0, -1.0, 0.0);
				vertices.push_back(ChunkVertex{{face.min.at(0), face.fixedCoord, face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.min.at(0), face.fixedCoord, face.min.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.fixedCoord, face.max.at(1)}, normal, colors.at(face.type)});
				vertices.push_back(ChunkVertex{{face.max.at(0), face.fixedCoord, face.min.at(1)}, normal, colors.at(face.type)});
			} break;
			default: throw std::runtime_error("Extra direction?!");
		}

		indices.push_back(baseIndex + 1);
		indices.push_back(baseIndex);
		indices.push_back(baseIndex + 3);
		indices.push_back(baseIndex + 3);
		indices.push_back(baseIndex);
		indices.push_back(baseIndex + 2);
	}

	for (ChunkVertex& vert : vertices) {
		vert.position = vert.position + glm::vec3(box.min);
		vert.position.z = -vert.position.z;
	}

	return {vertices, indices};
}

void Chunk::printStats() {
	//std::cout << "Tree loads: " << "\n";
	//regions.printCounts();
	std::cout << "Regions: " << regions.size() << ", Nodes: " << regions.getNodeCount() << ", Size: " << regions.getMemUsage() << " bytes\n";
}
