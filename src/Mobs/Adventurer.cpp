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

#include "Adventurer.hpp"
#include "../PlayerInputComponent.hpp"
#include "ScreenComponents.hpp"
#include "../Names.hpp"

namespace {
	const std::array<glm::vec3, 3> attackRots = {
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(0.0, -1.0, 0.0),
		glm::vec3(-1.0, 1.0, 0.0)
	};
}

void Adventurer::attack() {
	std::shared_ptr<MobState> state = getState();

	if (state->attackCooldown == 0) {
		state->attackEnd = 30;
		state->attackTime = 30;
		state->attackNum = 0;
		state->attackCooldown = 35;
	}
	else if (state->attackNum < 2 && state->attackCooldown > 0 && state->attackTime < 15) {
		state->attackEnd -= 5;
		state->attackTime = state->attackEnd;
		state->attackNum++;
		state->attackCooldown = 35;
	}
}

void Adventurer::update(Screen* screen) {
	Mob::update(screen);

	std::shared_ptr<MobState> state = getState();

	if (state->attackTime > 0) {
		constexpr float maxRot = 0.9f * ExMath::PI;
		constexpr float weaponLength = 2.0f;

		float angle = (1.0f - (float) state->attackTime / state->attackEnd) * maxRot - (maxRot / 2.0f);

		glm::mat4 rot = glm::rotate(glm::mat4_cast(state->rotation), angle, attackRots.at(state->attackNum));
		glm::vec3 end = glm::vec3(rot * glm::vec4(0.0, 0.0, -weaponLength, 0.0));

		std::shared_ptr<PhysicsComponent> physics = getPhysics();

		std::vector<PhysicsComponent*> hits = getPhysicsWorld(screen)->raytraceAll(physics->getTranslation(), physics->getTranslation() + end);

		for (PhysicsComponent* hit : hits) {
			glm::vec3 impulse = glm::normalize(hit->getTranslation() - physics->getTranslation()) * 0.1f;
			hit->applyImpulse(impulse);
		}

		float startRot = -(maxRot / 2.0f);
		float endRot = (1.0f * maxRot) - (maxRot / 2.0f);
		glm::vec3 start(glm::rotate(glm::mat4_cast(state->rotation), startRot, attackRots.at(state->attackNum)) * glm::vec4(0.0, 0.0, -weaponLength, 0.0));
		glm::vec3 finish(glm::rotate(glm::mat4_cast(state->rotation), endRot, attackRots.at(state->attackNum)) * glm::vec4(0.0, 0.0, -weaponLength, 0.0));
		getPhysicsWorld(screen)->drawDebugLine(physics->getTranslation(), physics->getTranslation() + start, glm::vec3(1.0, 1.0, 0.0));
		getPhysicsWorld(screen)->drawDebugLine(physics->getTranslation(), physics->getTranslation() + finish, glm::vec3(0.0, 1.0, 0.0));
	}
}

std::shared_ptr<Object> Adventurer::create() {
	std::shared_ptr<Object> adventurer = std::make_shared<Object>();

	PhysicsInfo capsulePhysics = {
		.shape = PhysicsShape::CAPSULE,
		.box = Aabb<float>({-0.5, -0.325, -0.5}, {0.5, 0.325, 0.5}),
		.pos = {1.0, 300.875, 0.0},
		.mass = 0.7f,
		.friction = 0.3f,
		.disableRotation = true,
	};

	std::shared_ptr<PhysicsComponent> advenPhysics = std::make_shared<PhysicsComponent>(std::make_shared<PhysicsObject>(capsulePhysics));
	advenPhysics->setAcceleration(2.5);
	adventurer->addComponent(advenPhysics);

	MobState state = {};
	state.stats = {
		.speed = 7.6f,
		.jumpStrength = 4.5f,
	};

	adventurer->setState<MobState>(state);

	//TODO: Control type of input - controlled or AI
	adventurer->addComponent<PlayerInputComponent>();
	adventurer->addComponent<Adventurer>();
	adventurer->addComponent<RenderComponent>(PLAYER_MAT, PLAYER_MESH);

	return adventurer;
}
