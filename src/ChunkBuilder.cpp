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

#include "ChunkBuilder.hpp"

void ChunkBuilder::addRegion(const Region& reg) {
	if (!box.contains(reg.box)) {
		std::cout << reg.box << "\n";
		throw std::invalid_argument("Attempt to add region not within chunk!");
	}

	if (reg.box.getVolume() == 0) {
		std::cout << reg.box << "\n";
		throw std::runtime_error("Attempted to add 0-volume box!");
	}

	Pos_t min = reg.box.min - box.min;
	Pos_t max = reg.box.max - box.min - Pos_t(1, 1, 1);

	InternalRegion region = {
		.type = reg.type,
		.box = Aabb<uint8_t>(min, max),
	};

	regions.push_back(region);
}
