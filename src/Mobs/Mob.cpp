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

#include "Mob.hpp"

void Mob::move(glm::vec3 direction) {
	if (getState()->isOnGround()) {
		glm::vec3 adjDir = direction;

		if (glm::length(direction) != 0.0f) {
			//Adjust direction for smooth turning
			adjDir = getState()->stats.speed * glm::normalize(direction);
			triedMove = true;

			glm::vec3 currentVel = getPhysics()->getVelocity();

			if (glm::length(adjDir - currentVel) > 0.01f) {
				adjDir = adjDir - currentVel;
			}

			//Update object rotation, if not targeting
			if (!target.lock()) {
				std::shared_ptr<MobState> state = getState();
				state->rotation = glm::conjugate(glm::quat(glm::lookAt(glm::vec3(0.0, 0.0, 0.0), glm::normalize(direction), glm::vec3(0.0, 1.0, 0.0))));
			}
		}

		getPhysics()->setVelocity(adjDir);
	}
}

void Mob::jump(glm::vec3 direction) {
	std::shared_ptr<MobState> state = getState();

	if (state->jumpCooldown == 0 && state->isOnGround()) {
		getPhysics()->setLinearDamping(0.0);
		getPhysics()->applyImpulse(state->stats.jumpStrength * glm::vec3(0.0, 1.0, 0.0));
		state->jumpCooldown = 30;
	}
}

void Mob::setTarget(PhysicsComponent* comp) {
	if (comp && comp->getParent() != lockParent() && comp->getParent()->getComponent<Mob>(UPDATE_COMPONENT_NAME)) {
		target = comp->getParent();
	}
	else {
		target.reset();
	}
}

void Mob::update(Screen* screen) {
	std::shared_ptr<MobState> state = getState();
	std::shared_ptr<PhysicsComponentManager> world = getPhysicsWorld(screen);
	std::shared_ptr<PhysicsComponent> physics = getPhysics();

	glm::vec3 pos = physics->getTranslation();

	bool onGround = world->raytraceSingle(pos, pos - glm::vec3(0.0, 0.876, 0.0)).hitComp != nullptr ||
					world->raytraceSingle(pos + glm::vec3(-0.25, 0.0, -0.25), pos + glm::vec3(-0.35, -0.78, -0.35)).hitComp != nullptr ||
					world->raytraceSingle(pos + glm::vec3(0.25, 0.0, 0.25), pos + glm::vec3(0.35, -0.78, 0.35)).hitComp != nullptr ||
					world->raytraceSingle(pos + glm::vec3(-0.25, 0.0, 0.25), pos + glm::vec3(-0.35, -0.78, 0.35)).hitComp != nullptr ||
					world->raytraceSingle(pos + glm::vec3(0.25, 0.0, -0.25), pos + glm::vec3(0.35, -0.78, -0.35)).hitComp != nullptr;

	state->flags.set(MobState::Flags::ON_GROUND, onGround);

	if (!onGround) {
		physics->setVelocity(glm::vec3(0.0, 0.0, 0.0));
	}

	if (state->jumpCooldown > 0) {
		state->jumpCooldown--;
	}

	if (state->attackTime > 0) {
		state->attackTime--;
	}

	if (state->attackCooldown > 0) {
		state->attackCooldown--;
	}

	if (triedMove || !state->isOnGround()) {
		physics->setLinearDamping(0.0f);
	}
	else {
		physics->setLinearDamping(0.999f);
	}

	triedMove = false;

	//If targeting an object, turn to face it.
	if (target.lock()) {
		glm::vec3 targetPos = target.lock()->getComponent<Mob>(UPDATE_COMPONENT_NAME)->getTarget()->getTranslation();
		state->rotation = glm::conjugate(glm::quat(glm::lookAt(pos, targetPos, glm::vec3(0.0, 1.0, 0.0))));
	}

	//Misc. debug drawing
	glm::vec3 rotVec = glm::vec3(glm::mat4_cast(state->rotation) * glm::vec4(0.0, 0.0, -1.0, 0.0));
	world->drawDebugLine(physics->getTranslation(), physics->getTranslation() + rotVec, glm::vec3(0.0, 1.0, 1.0));
}
