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

#include "Mob.hpp"

class Adventurer : public Mob {
public:
	/**
	 * Creates an adventurer object. Use create instead.
	 */
	Adventurer() : Mob() {}

	/**
	 * Attacks with whatever weapon the adventurer happens to be holding.
	 */
	void attack() override;

	/**
	 * Updates attack timers and such.
	 * @param screen The parent screen for the object.
	 */
	void update(Screen* screen) override;

	/**
	 * Creates a new adventurer object, for adding to the world.
	 * @return A new adventurer to add to the world.
	 */
	static std::shared_ptr<Object> create(/** TODO: Generation parameters go here **/);
};
