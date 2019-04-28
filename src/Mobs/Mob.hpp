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

#include <memory>

#include "Components/UpdateComponent.hpp"
#include "MobState.hpp"
#include "Components/PhysicsComponentManager.hpp"

class Mob : public UpdateComponent {
public:
	/**
	 * Creates a mob. Arguements are the same as those in UpdateComponent.
	 * @param startingState The state to start in.
	 * @param startingTime The time to start sleeping for, if starting state is SLEEPING.
	 * @param concurrent Whether the mob can be updated concurrently.
	 */
	Mob(UpdateState startingState = UpdateState::ACTIVE, size_t startingTime = 0, bool concurrent = false) :
		UpdateComponent(startingState, startingTime, concurrent),
		triedMove(false) {}

	/**
	 * Command function for the AI to communicate with the mob.
	 * Tells the mob to try moving in the given direction.
	 * @param direction A direction vector.
	 */
	void move(glm::vec3 direction);

	/**
	 * AI communication function. Tells the mob to try jumping in the given direction.
	 * @param direction The direction to jump.
	 */
	void jump(glm::vec3 direction);

	/**
	 * Asks the mob to attack, result is mob dependent.
	 */
	virtual void attack() {}

	/**
	 * Sets the mob to target the provided component, making its movement relative
	 * to it.
	 * @param comp The object to target, or nullptr to clear the set target.
	 */
	void setTarget(PhysicsComponent* comp);

	/**
	 * Returns the physics component of the object this mob is targeting.
	 * @return The targeted physics component, or nullptr if there is no target.
	 */
	PhysicsComponent* getTarget() { return target.lock() ? target.lock()->getComponent<PhysicsComponent>(PHYSICS_COMPONENT_NAME).get() : nullptr; }

	/**
	 * Gets the parent object's state.
	 * @return The set state for the parent object.
	 */
	std::shared_ptr<MobState> getState() {
		return lockParent()->getState<MobState>();
	}

	/**
	 * Gets the physics component from the parent object.
	 * @return The physics component.
	 */
	std::shared_ptr<PhysicsComponent> getPhysics() {
		return lockParent()->getComponent<PhysicsComponent>(PHYSICS_COMPONENT_NAME);
	}

	/**
	 * Gets the physics component manager for the parent screen.
	 * @param screen The screen to fetch the manager from.
	 * @return The physics component manager for the given screen.
	 */
	std::shared_ptr<PhysicsComponentManager> getPhysicsWorld(Screen* screen) {
		return std::static_pointer_cast<PhysicsComponentManager>(screen->getManager(PHYSICS_COMPONENT_NAME));
	}

	/**
	 * Updates the object's state.
	 * @param screen The parent screen.
	 */
	void update(Screen* screen) override;

private:
	//Whether the AI tried to move in the last tick.
	bool triedMove;
	//Targeted object.
	std::weak_ptr<Object> target;
};
