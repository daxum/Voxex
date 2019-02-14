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
#include "Engine.hpp"
#include "Names.hpp"
#include "Voxex.hpp"

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

ChunkMeshData Chunk::generateModel(const std::array<std::array<float, 3>, 20>& colors) {
	std::vector<RegionFace> faces = regions.genQuads();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	//TODO: find better way to get this sort of stuff
	VertexBuffer& buffer = Engine::instance->getModelManager().getMemoryManager()->getBuffer(CHUNK_BUFFER);

	for (const RegionFace& face : faces) {
		size_t baseIndex = vertices.size();

		glm::vec3 normal;
		std::array<glm::vec3, 4> positions;

		switch (face.getNormal()) {
			//Facing -z
			case 0: {
				normal = glm::vec3(0.0, 0.0, -1.0);
				positions.at(0) = {face.min.at(0), face.min.at(1), face.fixedCoord};
				positions.at(1) = {face.min.at(0), face.max.at(1), face.fixedCoord};
				positions.at(2) = {face.max.at(0), face.min.at(1), face.fixedCoord};
				positions.at(3) = {face.max.at(0), face.max.at(1), face.fixedCoord};
			} break;
			//Facing -x
			case 1: {
				normal = glm::vec3(-1.0, 0.0, 0.0);
				positions.at(0) = {face.fixedCoord, face.min.at(0), face.max.at(1)};
				positions.at(1) = {face.fixedCoord, face.max.at(0), face.max.at(1)};
				positions.at(2) = {face.fixedCoord, face.min.at(0), face.min.at(1)};
				positions.at(3) = {face.fixedCoord, face.max.at(0), face.min.at(1)};
			} break;
			//Facing +z
			case 2: {
				normal = glm::vec3(0.0, 0.0, 1.0);
				positions.at(0) = {face.max.at(0), face.min.at(1), face.fixedCoord};
				positions.at(1) = {face.max.at(0), face.max.at(1), face.fixedCoord};
				positions.at(2) = {face.min.at(0), face.min.at(1), face.fixedCoord};
				positions.at(3) = {face.min.at(0), face.max.at(1), face.fixedCoord};
			} break;
			//Facing +x
			case 3: {
				normal = glm::vec3(1.0, 0.0, 0.0);
				positions.at(0) = {face.fixedCoord, face.max.at(0), face.max.at(1)};
				positions.at(1) = {face.fixedCoord, face.min.at(0), face.max.at(1)};
				positions.at(2) = {face.fixedCoord, face.max.at(0), face.min.at(1)};
				positions.at(3) = {face.fixedCoord, face.min.at(0), face.min.at(1)};
			} break;
			//Facing +y
			case 4: {
				normal = glm::vec3(0.0, 1.0, 0.0);
				positions.at(0) = {face.min.at(0), face.fixedCoord, face.min.at(1)};
				positions.at(1) = {face.min.at(0), face.fixedCoord, face.max.at(1)};
				positions.at(2) = {face.max.at(0), face.fixedCoord, face.min.at(1)};
				positions.at(3) = {face.max.at(0), face.fixedCoord, face.max.at(1)};
			} break;
			//Facing -y
			case 5: {
				normal = glm::vec3(0.0, -1.0, 0.0);
				positions.at(0) = {face.min.at(0), face.fixedCoord, face.max.at(1)};
				positions.at(1) = {face.min.at(0), face.fixedCoord, face.min.at(1)};
				positions.at(2) = {face.max.at(0), face.fixedCoord, face.max.at(1)};
				positions.at(3) = {face.max.at(0), face.fixedCoord, face.min.at(1)};
			} break;
			default: throw std::runtime_error("Extra direction?!");
		}

		for (size_t i = 0; i < positions.size(); i++) {
			//Center the chunk, invert z
			positions.at(i) -= glm::vec3(128, 128, 128);
			positions.at(i).z = -positions.at(i).z;

			glm::vec3 color(colors.at(face.type).at(0), colors.at(face.type).at(1), colors.at(face.type).at(2));

			Vertex vert = buffer.getVertex();
			vert.setVec3(VERTEX_ELEMENT_POSITION, positions.at(i));
			vert.setVec3(VERTEX_ELEMENT_NORMAL, normal);
			vert.setVec3(VERTEX_ELEMENT_COLOR, color);
			vertices.push_back(vert);
		}

		indices.push_back(baseIndex + 1);
		indices.push_back(baseIndex);
		indices.push_back(baseIndex + 3);
		indices.push_back(baseIndex + 3);
		indices.push_back(baseIndex);
		indices.push_back(baseIndex + 2);
	}

	std::string modelName = "Chunk_" + std::to_string(box.min.x) + "_" + std::to_string(box.min.y) + "_" + std::to_string(box.min.z);

	ChunkMeshData out = {
		modelName,
		Mesh(CHUNK_BUFFER, {
			{VERTEX_ELEMENT_POSITION, VertexElementType::VEC3},
			{VERTEX_ELEMENT_NORMAL, VertexElementType::VEC3},
			{VERTEX_ELEMENT_COLOR, VertexElementType::VEC3}},
			vertices,
			indices,
			box,
			std::sqrt(std::sqrt(255*255 + 255*255)*std::sqrt(255*255 + 255*255) + 255*255)),
		Model(modelName, modelName, CHUNK_SHADER, CHUNK_SET, Voxex::chunkSet)
	};

	return out;
}

void Chunk::printStats() {
	//std::cout << "Tree loads: " << "\n";
	//regions.printCounts();
	std::cout << "Regions: " << regions.size() << ", Nodes: " << regions.getNodeCount() << ", Size: " << regions.getMemUsage() << " bytes\n";
}
