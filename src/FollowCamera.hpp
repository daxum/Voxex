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

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"
#include "Object.hpp"
#include "Screen.hpp"
#include "ExtraMath.hpp"
#include "Mobs/Mob.hpp"

class FollowCamera : public Camera {
public:
	FollowCamera(std::shared_ptr<Object> tar, Screen* parent) :
		target(tar),
		parent(parent),
		cameraPos(tar->getComponent<Mob>(UPDATE_COMPONENT_NAME)->getFocalPoint()),
		cameraLook(tar->getComponent<PhysicsComponent>(PHYSICS_COMPONENT_NAME)->getTranslation()),
		near(0.1f),
		far(10'000.0f) {}

	const glm::mat4 getView() const override {
		return glm::lookAt(cameraPos, cameraLook, glm::vec3(0.0, 1.0, 0.0));
	}

	const glm::mat4 getProjection() const override { return projection; }

	void setProjection() override {
		float width = Engine::instance->getWindowInterface().getWindowWidth();
		float height = Engine::instance->getWindowInterface().getWindowHeight();

		projection = glm::perspective(ExMath::PI / 4.0f, width / height, near, far);
	}

	std::pair<float, float> getNearFar() const override { return {near, far}; }

	float getFOV() const override { return ExMath::PI / 4.0f; }

	void update() override {
		std::shared_ptr<Mob> mob = target->getComponent<Mob>(UPDATE_COMPONENT_NAME);
		std::shared_ptr<PhysicsComponent> physics = mob->getPhysics();

		cameraPos += -0.3f * (cameraPos - (mob->getFocalPoint()));
		cameraPos.y = physics->getTranslation().y + 3.0;
		cameraLook += -0.3f * (cameraLook - physics->getTranslation());
	}

private:
	std::shared_ptr<Object> target;
	Screen* parent;

	glm::vec3 cameraPos;
	glm::vec3 cameraLook;

	glm::mat4 projection;
	float near;
	float far;
};
