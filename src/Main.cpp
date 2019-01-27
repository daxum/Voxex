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

#include <stack>
#include <atomic>
#include <mutex>

#include "tbb/parallel_for.h"

#include "Renderer.hpp"
#include "ExtraMath.hpp"
#include "AxisAlignedBB.hpp"
#include "Chunk.hpp"

std::ostream& operator<<(std::ostream& out, const glm::vec3& v) {
	return out << "vec3[" << v.x << " " << v.y << " " << v.z << "]";
}

std::ostream& operator<<(std::ostream& out, const glm::vec2& v) {
	return out << "vec2[" << v.x << " " << v.y << "]";
}

static const std::array<std::array<float, 3>, 20> colors = {
	0.200f, 0.200f, 0.200f, //0
	0.430f, 0.366f, 0.075f, //1
	0.100f, 0.900f, 0.150f, //2
	0.900f, 0.010f, 0.200f, //3
	0.010f, 0.300f, 0.950f, //4
	0.500f, 0.500f, 0.300f, //5
	0.300f, 0.750f, 0.800f, //6
	1.000f, 1.000f, 1.000f, //7
	1.000f, 0.000f, 0.000f, //8
	0.000f, 1.000f, 0.000f, //9
	0.000f, 0.000f, 1.000f, //10
	0.100f, 0.100f, 0.100f, //11
	0.200f, 0.200f, 0.200f, //12
	0.300f, 0.300f, 0.300f, //13
	0.400f, 0.400f, 0.400f, //14
	0.500f, 0.500f, 0.500f, //15
	0.600f, 0.600f, 0.600f, //16
	0.700f, 0.700f, 0.700f, //17
	0.800f, 0.800f, 0.800f, //18
	0.900f, 0.900f, 0.900f, //19
};

static const std::string seed = "Faster rendering!";

//TODO: better hash
glm::vec3 getGradient(std::array<int64_t, 3> pos) {
	glm::vec3 out;

	int64_t seedHash = (int64_t) std::hash<std::string>()(seed);

	int64_t val = (pos.at(0) + 2) * 7;
	val ^= (pos.at(1) + 3) * 13;
	val ^= (pos.at(2) + 5) * 17;

	for (size_t i = 0; i < pos.size(); i++) {
		out[i] = (pos.at(i) ^ val) * (seedHash + 13);
	}

	//Avoid becoming too big in the dot product and returning infinity (makes normalize return 0)
	out /= 4'000'000'000;

	return glm::normalize(out);
}

float smooth(float val) {
	return val * val * val * (val * (val * 6.0f - 15.0f) + 10.0f);
}

float perlin1D(int64_t point) {
	const int64_t gridScale = 32;

	std::array<float, 2> corners = {
		(float)(point / gridScale * gridScale),
		(float)(point / gridScale * gridScale + gridScale)
	};

	if (point < 0) {
		corners.at(0) -= gridScale;
		corners.at(1) -= gridScale;
	}

	std::array<float, 2> cornerGrads;

	for (size_t i = 0; i < cornerGrads.size(); i++) {
		cornerGrads.at(i) = getGradient({(int64_t)corners.at(i), 0, 0}).x;
	}

	float xPercent = smooth((point - corners.at(0)) / (corners.at(1) - corners.at(0)));

	return (ExMath::interpolate<float>(cornerGrads.at(0), cornerGrads.at(1), xPercent) + 1.0f) / 2.0f;
}

float perlin2D(std::array<int64_t, 2> point) {
	const int64_t gridScale = 512;

	glm::vec2 min(point.at(0) / gridScale * gridScale, point.at(1) / gridScale * gridScale);
	glm::vec2 max(point.at(0) / gridScale * gridScale + gridScale, point.at(1) / gridScale * gridScale + gridScale);

	if (point.at(0) < 0) {
		min.x -= gridScale;
		max.x -= gridScale;
	}

	if (point.at(1) < 0) {
		min.y -= gridScale;
		max.y -= gridScale;
	}

	std::array<glm::vec2, 4> corners = {
		min,
		glm::vec2(max.x, min.y),
		glm::vec2(min.x, max.y),
		max
	};

	std::array<glm::vec2, 4> cornerGrads;

	for (size_t i = 0; i < cornerGrads.size(); i++) {
		cornerGrads.at(i) = getGradient({(int64_t)corners.at(i).x, (int64_t)corners.at(i).y, 0});
	}

	glm::vec2 fPoint(
		point.at(0),
		point.at(1)
	);

	std::array<float, 4> dotProducts;

	for (size_t i = 0; i < dotProducts.size(); i++) {
		if (fPoint != corners.at(i)) {
			dotProducts.at(i) = glm::dot(cornerGrads.at(i), (fPoint - corners.at(i))/glm::length(glm::vec2(gridScale, gridScale)));
		}
		else {
			dotProducts.at(i) = 0.0f;
		}
	}

	float xPercent = smooth((fPoint.x - min.x) / (max.x-min.x));
	float yPercent = smooth((fPoint.y - min.y) / (max.y-min.y));

	return (ExMath::bilinearInterpolate<float>(dotProducts, xPercent, yPercent) + 1.0f) / 2.0f;
}

float perlin3D(Aabb<int64_t>::vec_t point, int64_t gridScale) {
	Aabb<int64_t>::vec_t negAdj(0, 0, 0);

	if (point.x < 0) negAdj.x = gridScale;
	if (point.y < 0) negAdj.y = gridScale;
	if (point.z < 0) negAdj.z = gridScale;

	Aabb<int64_t> box(
		{point.x / gridScale * gridScale, point.y / gridScale * gridScale, point.z / gridScale * gridScale},
		{point.x / gridScale * gridScale + gridScale, point.y / gridScale * gridScale + gridScale, point.z / gridScale * gridScale + gridScale}
	);

	box.min -= negAdj;
	box.max -= negAdj;

	std::array<glm::vec3, 8> corners = {
		glm::vec3(box.min.x, box.min.y, box.min.z),
		glm::vec3(box.max.x, box.min.y, box.min.z),
		glm::vec3(box.min.x, box.min.y, box.max.z),
		glm::vec3(box.max.x, box.min.y, box.max.z),
		glm::vec3(box.min.x, box.max.y, box.min.z),
		glm::vec3(box.max.x, box.max.y, box.min.z),
		glm::vec3(box.min.x, box.max.y, box.max.z),
		glm::vec3(box.max.x, box.max.y, box.max.z)
	};

	std::array<glm::vec3, 8> cornerGrads;

	for (size_t i = 0; i < cornerGrads.size(); i++) {
		cornerGrads.at(i) = getGradient({(int64_t)corners.at(i).x, (int64_t)corners.at(i).y, (int64_t)corners.at(i).z});
	}

	glm::vec3 fPoint(
		(float)point.x,
		(float)point.y,
		(float)point.z
	);

	std::array<float, 8> dotProducts;

	for (size_t i = 0; i < dotProducts.size(); i++) {
		if (fPoint != corners.at(i)) {
			dotProducts.at(i) = glm::dot(cornerGrads.at(i), (fPoint - corners.at(i))/glm::length(glm::vec3(gridScale, gridScale, gridScale)));
		}
		else {
			dotProducts.at(i) = 0.0f;
		}
	}

	float xPercent = smooth((fPoint.x - box.min.x) / box.xLength());
	float yPercent = smooth((fPoint.y - box.min.y) / box.yLength());
	float zPercent = smooth((fPoint.z - box.min.z) / box.zLength());

	return (ExMath::trilinearInterpolate<float>(dotProducts, xPercent, zPercent, yPercent) + 1.0f) / 2.0f;
}

float fillPercent(const Aabb<int64_t>& mainBox) {
	return perlin3D(mainBox.getCenter(), 256);
}

Chunk genChunk(const Pos_t& pos) {
	Chunk chunk(pos);

	std::stack<Aabb<int64_t>> regions;
	regions.push(Aabb<int64_t>(chunk.getBox().min, chunk.getBox().max - Pos_t(1, 1, 1)));
	uint64_t boxes = 0;

	while (!regions.empty()) {
		boxes++;

		Aabb<int64_t> box = regions.top();
		regions.pop();

		constexpr int64_t minEdge = 1;
		constexpr float fillThreshold = 0.20f;
		constexpr float cutoffScale = 72.0f;
		constexpr float discardThreshold = 0.37f;

		float percentFull = fillPercent(box);
		float adjThreshold = ExMath::clamp(cutoffScale * (1.0f / (256.0f - box.xLength())) + fillThreshold, 0.0f, 0.95f);

		if (percentFull >= adjThreshold) {
			uint16_t type = 5;//(uint16_t)ExMath::randomInt(0, colors.size() - 1);
			chunk.addRegion(Region{type, box});
		}
		else if (box.xLength() > minEdge && percentFull > discardThreshold) {
			std::array<Aabb<int64_t>, 8> toAdd = box.split();

			for (Aabb<int64_t> add : toAdd) {
				if (add.getVolume() > 0) {
					regions.push(add);
				}
			}
		}
	}

	return chunk;
}

Chunk worldGenChunk(const Pos_t& pos) {
	Chunk chunk(pos);
	Aabb<int64_t> chunkBox = chunk.getBox();

	//Ground - anything below 0 is underground
	if (chunkBox.max.y <= 0) {
		Aabb<int64_t> groundBox = chunkBox;
		//Subtract 1 from max to prevent 8-bit overflow
		groundBox.max -= Pos_t(1, 1, 1);

		chunk.addRegion({16, groundBox});
	}

	//Add layer of dirt and stone for terrain
	else if (chunkBox.max.y <= 256 && chunkBox.min.y >= 0) {
		for (int64_t i = pos.x; i < chunkBox.max.x-1; i++) {
			for (int64_t j = pos.z; j < chunkBox.max.z-1; j++) {
				float heightPercent = perlin2D({i, j});
				int64_t height = (int64_t) (heightPercent * 255) + pos.y;
				int64_t stoneHeight = pos.y + height / 2;

				if (stoneHeight > 0) {
					chunk.addRegion({16, Aabb<int64_t>({i, 0, j}, {i+1, stoneHeight, j+1})});
				}

				chunk.addRegion({2, Aabb<int64_t>({i, stoneHeight, j}, {i+1, height, j+1})});
			}
		}
	}

	return chunk;
}

int main(int argc, char** argv) {
	std::vector<Chunk> chunks;
	std::mutex chunkLock;

	constexpr size_t maxI = 4;
	constexpr size_t maxJ = 4;
	constexpr size_t maxK = 4;
	constexpr size_t maxChunks = maxI * maxJ * maxK;

	std::atomic<double> genTime(0.0);
	double createTime = ExMath::getTimeMillis();

	tbb::parallel_for(size_t(0), maxChunks, [&](size_t val) {
		size_t i = (val / (maxK * maxJ)) % maxI;
		size_t j = val / maxK % maxJ;
		size_t k = val % maxK;

		double start = ExMath::getTimeMillis();
		Chunk chunk = genChunk(Pos_t{256*i-256*(maxI/2), 256*j-256*(maxJ/2), 256*k-256*(maxK/2)});
		double end = ExMath::getTimeMillis();

		std::cout << "Generated in " << end - start << "ms\n";

		double genExpect = 0.0;
		double genAdd = 0.0;

		do {
			genExpect = genTime.load();
			genAdd = end - start + genExpect;
		} while (!genTime.compare_exchange_strong(genExpect, genAdd, std::memory_order_relaxed));

		size_t oldCount = chunk.regionCount();

		std::cout << "Starting with " << chunk.regionCount() << " regions!\n";
		chunk.printStats();

		chunk.optimize();
		size_t pass = 2;

		while(oldCount != chunk.regionCount()) {
			oldCount = chunk.regionCount();
			chunk.optimize();
			pass++;
		}

		std::cout << "Reduced to " << chunk.regionCount() << " regions in " << pass << " passes\n";

		chunk.validate();
		chunk.printStats();

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			chunks.push_back(chunk);
		}
	});

	createTime = ExMath::getTimeMillis() - createTime;

	size_t totalRegions = 0;
	size_t totalMemUsage = 0;

	for (size_t i = 0; i < chunks.size(); i++) {
		totalRegions += chunks.at(i).regionCount();
		totalMemUsage += chunks.at(i).getMemUsage();
	}


	std::cout << "Generated in " << genTime.load() << "ms (" << genTime.load() / (maxI*maxJ*maxK) << "ms per chunk)!\n";
	std::cout << "Generation completed in " << createTime << "ms\n";
	std::cout << "Generated " << totalRegions << " regions\n";
	//Yes, I know the correct term
	std::cout << "Chunks using around " << (totalMemUsage >> 10) << " kilobytes in total\n";

	Render renderer;

	tbb::parallel_for(size_t(0), chunks.size(), [&](size_t val) {
		auto data = chunks.at(val).generateMesh(colors);

		{
			std::lock_guard<std::mutex> guard(chunkLock);
			renderer.addChunkData(data);
		}

		std::cout << "Chunk " << val << " complete!\n";
	});

	renderer.upload();

	while(!renderer.shouldExit()) {
		renderer.pollEvents();
		renderer.render();
		renderer.present();
	}

	return 0;
}
