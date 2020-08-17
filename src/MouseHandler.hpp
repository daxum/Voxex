/******************************************************************************
 * Voxex - An experiment with sparse voxel terrain
 * Copyright (C) 2019, 2020
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

#include "Components/UpdateComponent.hpp"
#include "Display/ObjectPhysicsInterface.hpp"
#include "ChunkLoader.hpp"
#include "Components/PhysicsManager.hpp"

class MouseHandler : public UpdateComponent, ObjectPhysicsInterface {
public:
	/**
	 * Constructor. Does next to nothing.
	 * @param loader The world's chunk loader.
	 */
	MouseHandler(std::shared_ptr<ChunkLoader> loader) : chunkLoader(loader), boxPos(0.0, 0.0, 0.0) {}

	/**
	 * Moves or hides the selection box depending on where the mouse is on the screen.
	 * @param screen The screen passed in from the manager.
	 */
	void update(Screen* screen) override;

	/**
	 * Sets the mouse handler as the physics provider.
	 */
	void onParentSet() override { lockParent()->setPhysics(this); }

	/**
	 * Returns the position of the mouse selection box.
	 */
	glm::vec3 getTranslation() const override { return boxPos; }

private:
	//The chunk loader, used for accessing loaded chunks.
	std::shared_ptr<ChunkLoader> chunkLoader;
	//Box which highlights which block the mouse is hovering over.
	glm::vec3 boxPos;
};
