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

#include <array>
#include <cstdint>

#include "AxisAlignedBB.hpp"

float perlin1D(int64_t point);

float perlin2D(std::array<int64_t, 2> point, int64_t gridScale, uint64_t seedHash);

float perlin2DOctaves(std::array<int64_t, 2> point, uint64_t octaves, int64_t gridScale, uint64_t seedHash);

float perlin3D(Aabb<int64_t>::vec_t point, int64_t gridScale);
