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

#include <memory>

#include "Chunk.hpp"
#include "Engine.hpp"
#include "Names.hpp"
#include "Voxex.hpp"

namespace {
	struct ChunkVert {
		glm::vec3 pos;
		uint32_t normColPack;
	};
}

ChunkMeshData Chunk::generateMesh() {
	double start = ExMath::getTimeMillis();

	std::vector<RegionFace> faces = regions.genQuads();

	double end = ExMath::getTimeMillis();

	std::unique_ptr<unsigned char[]> vertexData = std::make_unique<unsigned char[]>(faces.size() * 4 * sizeof(ChunkVert));
	size_t lastVertex = 0;
	std::vector<uint32_t> indices(faces.size() * 6, 0);

	for (const RegionFace& face : faces) {
		size_t baseIndex = lastVertex;

		std::array<glm::vec3, 4> positions;

		std::array<float, 2> min = {(float)face.min.at(0), (float)face.min.at(1)};
		std::array<float, 2> max = {(float)face.max.at(0), (float)face.max.at(1)};
		float fixedCoord = face.getFixedCoord();

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
				positions.at(0) = {fixedCoord, max.at(0), max.at(1)};
				positions.at(1) = {fixedCoord, min.at(0), max.at(1)};
				positions.at(2) = {fixedCoord, max.at(0), min.at(1)};
				positions.at(3) = {fixedCoord, min.at(0), min.at(1)};
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
				positions.at(0) = {fixedCoord, min.at(0), max.at(1)};
				positions.at(1) = {fixedCoord, max.at(0), max.at(1)};
				positions.at(2) = {fixedCoord, min.at(0), min.at(1)};
				positions.at(3) = {fixedCoord, max.at(0), min.at(1)};
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

			//Copy in vertex data
			struct {
				glm::vec3 pos;
				uint32_t normColPack;
			} vert;

			vert.pos = positions.at(i);

			uint32_t normal = face.getNormal();
			uint32_t type = face.type;

			vert.normColPack = (normal << 16) | type;

			memcpy(&vertexData[lastVertex * sizeof(ChunkVert)], &vert, sizeof(ChunkVert));
			lastVertex++;
		}

		//When adding the indices, do two 32-bit values at a time for moderate speedup
		//The order of the added indices is 1, 0, 3, 3, 0, 2
		uint64_t* indexData = (uint64_t*) indices.data();
		size_t index = baseIndex / 4 * 3;

		uint64_t val = (((uint64_t)baseIndex) << 32) | baseIndex;

		indexData[index] = val + 1ul;
		indexData[index + 1] = val + 12884901891ul;
		indexData[index + 2] = val + 8589934592ul;
	}

	std::string modelName = "Chunk_" + std::to_string(box.min.x) + "_" + std::to_string(box.min.y) + "_" + std::to_string(box.min.z);

	Mesh::BufferInfo buffers = {
		.vertex = Engine::instance->getModelManager().getMemoryManager()->getBuffer(CHUNK_VERTEX_BUFFER),
		.index = Engine::instance->getModelManager().getMemoryManager()->getBuffer(CHUNK_INDEX_BUFFER),
		.vertexName = CHUNK_VERTEX_BUFFER,
		.indexName = CHUNK_INDEX_BUFFER,
	};

	//Distance from the center to the corner, calculated with many square roots
	float radius = 221.7025033688163f;
	const VertexFormat* format = Engine::instance->getModelManager().getFormat(CHUNK_FORMAT);

	ChunkMeshData out = {
		.name = modelName,
		.mesh = Mesh(buffers, format, vertexData.get(), lastVertex * sizeof(ChunkVert), indices, box, radius),
	};

	double vertEnd = ExMath::getTimeMillis();

	std::cout << "Reduced from " << (regions.size() * 6) << " to " << faces.size() << " faces - " <<
				 "completed in " << (end-start) << "ms\n";

	std::cout << "Vertex generation: " << (vertEnd - end) << "ms\n";
	std::cout << "Chunk mesh generation: " << (vertEnd - start) << "ms\n";

	return out;
}

void Chunk::printStats() {
	//std::cout << "Tree loads: " << "\n";
	//regions.printCounts();
	std::cout << "Regions: " << regions.size() << ", Nodes: " << regions.getNodeCount() << ", Size: " << regions.getMemUsage() << " bytes\n";
}
