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

#include "Perlin.hpp"
#include "ExtraMath.hpp"

namespace {
	const std::string seed = "Physics Enabled!";

	constexpr std::array<glm::vec3, 12> corners = {
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.0, 0.0, -1.0),
		glm::vec3(0.0, -1.0, 0.0),
		glm::vec3(-1.0, 0.0, 0.0),
		glm::vec3(1.0, 1.0, 0.0),
		glm::vec3(1.0, 0.0, 1.0),
		glm::vec3(0.0, 1.0, 1.0),
		glm::vec3(-1.0, -1.0, 0.0),
		glm::vec3(-1.0, 0.0, -1.0),
		glm::vec3(0.0, -1.0, -1.0)
	};

	glm::vec3 getGradient(std::array<int64_t, 3> pos) {
		uint64_t seedHash = std::hash<std::string>()(seed);

		uint64_t x = pos.at(0);
		uint64_t y = pos.at(1);
		uint64_t z = pos.at(2);

		uint64_t hash1 = ((x + y) * (x + y + 1)) / 2 + y;
		uint64_t hash2 = ((z + seedHash) * (z + seedHash + 1)) / 2 + seedHash;
		uint64_t hash = ((hash1 + hash2) * (hash1 + hash2 + 1)) / 2 + hash2;

		return glm::normalize(corners.at(hash % corners.size()));
	}

	constexpr float smooth(float val) {
		return val * val * val * (val * (val * 6.0f - 15.0f) + 10.0f);
	}
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
