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

ChunkMeshData Chunk::generateModel() {
	double start = ExMath::getTimeMillis();

	std::vector<RegionFace> faces = regions.genQuads();

	double end = ExMath::getTimeMillis();

	std::cout << "Reduced from " << (regions.size() * 6) << " to " << faces.size() << " faces - " <<
				 "completed in " << (end-start) << "ms\n";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	//TODO: find better way to get this sort of stuff
	VertexBuffer& buffer = Engine::instance->getModelManager().getMemoryManager()->getBuffer(CHUNK_BUFFER);

	for (const RegionFace& face : faces) {
		size_t baseIndex = vertices.size();

		std::array<glm::vec3, 4> positions;

		std::array<uint64_t, 2> min = {face.min.at(0), face.min.at(1)};
		std::array<uint64_t, 2> max = {face.max.at(0), face.max.at(1)};
		uint64_t fixedCoord = face.fixedCoord;

		switch (face.getNormal()) {
			//Facing -z
			case 0: {
				positions.at(0) = {min.at(0), min.at(1), fixedCoord};
				positions.at(1) = {min.at(0), max.at(1), fixedCoord};
				positions.at(2) = {max.at(0), min.at(1), fixedCoord};
				positions.at(3) = {max.at(0), max.at(1), fixedCoord};
			} break;
			//Facing -x
			case 1: {
				positions.at(0) = {fixedCoord, min.at(0), max.at(1)};
				positions.at(1) = {fixedCoord, max.at(0), max.at(1)};
				positions.at(2) = {fixedCoord, min.at(0), min.at(1)};
				positions.at(3) = {fixedCoord, max.at(0), min.at(1)};
			} break;
			//Facing +z
			case 2: {
				positions.at(0) = {max.at(0), min.at(1), fixedCoord};
				positions.at(1) = {max.at(0), max.at(1), fixedCoord};
				positions.at(2) = {min.at(0), min.at(1), fixedCoord};
				positions.at(3) = {min.at(0), max.at(1), fixedCoord};
			} break;
			//Facing +x
			case 3: {
				positions.at(0) = {fixedCoord, max.at(0), max.at(1)};
				positions.at(1) = {fixedCoord, min.at(0), max.at(1)};
				positions.at(2) = {fixedCoord, max.at(0), min.at(1)};
				positions.at(3) = {fixedCoord, min.at(0), min.at(1)};
			} break;
			//Facing +y
			case 4: {
				positions.at(0) = {min.at(0), fixedCoord, min.at(1)};
				positions.at(1) = {min.at(0), fixedCoord, max.at(1)};
				positions.at(2) = {max.at(0), fixedCoord, min.at(1)};
				positions.at(3) = {max.at(0), fixedCoord, max.at(1)};
			} break;
			//Facing -y
			case 5: {
				positions.at(0) = {min.at(0), fixedCoord, max.at(1)};
				positions.at(1) = {min.at(0), fixedCoord, min.at(1)};
				positions.at(2) = {max.at(0), fixedCoord, max.at(1)};
				positions.at(3) = {max.at(0), fixedCoord, min.at(1)};
			} break;
			default: throw std::runtime_error("Extra direction?!");
		}

		for (size_t i = 0; i < positions.size(); i++) {
			//Center the chunk, invert z
			positions.at(i) -= glm::vec3(128, 128, 128);
			positions.at(i).z = -positions.at(i).z;

			Vertex vert = buffer.getVertex();
			vert.setVec3(VERTEX_ELEMENT_POSITION, positions.at(i));

			uint32_t normal = face.getNormal();
			uint32_t type = face.type;

			vert.setUint32(VERTEX_ELEMENT_PACKED_NORM_COLOR, (normal << 16) | type);
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
			{VERTEX_ELEMENT_PACKED_NORM_COLOR, VertexElementType::UINT32}},
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
