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

#include "UpdateComponent.hpp"
#include "ExtraMath.hpp"

class BoxMonster : public Mob {
public:
	/**
	 * Creates a box monster. Use create instead.
	 */
	BoxMonster() : Mob(UpdateState::SLEEPING, ExMath::randomBinomialInt(60, 300, 210)) {}

	/**
	 * Creates a new box monster.
	 * @param pos The position of the box.
	 * @return The created monster.
	 */
	static std::shared_ptr<Object> create(glm::vec3 pos/** TODO: generation parameters **/ ) {
		PhysicsInfo boxPhysics = {
			.shape = PhysicsShape::BOX,
			.box = Aabb<float>({-0.5, -0.5, -0.5}, {0.5, 0.5, 0.5}),
			.pos = pos,
			.mass = 0.5f,
			.friction = 0.5f,
			.disableRotation = false,
		};

		std::shared_ptr<Object> box = std::make_shared<Object>();
		box->addComponent(std::make_shared<BoxMonster>());
		box->addComponent(std::make_shared<PhysicsComponent>(std::make_shared<PhysicsObject>(boxPhysics)));

		return box;
	}

	/**
	 * Called when the sleep timer runs out. Makes the monster jump a bit.
	 */
	void onWake() override {
		glm::vec3 dir(ExMath::randomFloat(-1.0f, 1.0f), 1.0f, ExMath::randomFloat(-1.0f, 1.0f));
		lockParent()->getComponent<PhysicsComponent>(PHYSICS_COMPONENT_NAME)->applyImpulse(2.0f * glm::normalize(dir));
		wakeTime = ExMath::randomBinomialInt(60, 300, 210);
	}
};
