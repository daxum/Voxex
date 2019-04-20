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

#include "Object.hpp"

struct MobStats {
	//How fast the mob moves, in m/s.
	//Average human walking speed: ~1.4 m/s
	//Average human running speed: ~3.8 m/s
	//Fastest human running speed: ~12.1 m/s
	float speed;
	//How far the mob can jump.
	float jumpStrength;
};

struct MobState : public ObjectState {
	//Possible state flags for the mob.
	enum Flags {
		ON_GROUND,
		NUM_FLAGS
	};

	//Current state of the monster.
	std::bitset<NUM_FLAGS> flags;

	//Cooldown before allowed to jump again.
	size_t jumpCooldown;

	//Attack timers - current and finish time.
	size_t attackEnd;
	size_t attackTime;
	//Attack combo number
	size_t attackNum;
	//Time before can attack again
	size_t attackCooldown;

	//Rotation of the mob, set using forward velocity.
	glm::quat rotation;

	MobStats stats;

	/**
	 * Gets any stuff needed for rendering, does nothing for now.
	 * @param name The value to get.
	 * @return a pointer to the retrieved value.
	 */
	const void* getRenderValue(const std::string& name) const override { return nullptr; }

	/**
	 * gets whether the mob is standing on something solid.
	 * @return Whether the mob is on the ground.
	 */
	bool isOnGround() { return flags.test(Flags::ON_GROUND); }
};
