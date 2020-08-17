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

#include <utility>

#include <glm/gtc/matrix_transform.hpp>

#include "Display/Camera.hpp"
#include "Display/Object.hpp"
#include "Display/Screen.hpp"
#include "ExtraMath.hpp"
#include "Mobs/Mob.hpp"
#include "Engine.hpp"

class FollowCamera : public Camera {
public:
	/**
	 * Creates a follow camera with the given object as its target.
	 * @param tar The target object.
	 */
	FollowCamera(std::shared_ptr<Object> tar) :
		target(tar),
		mob(tar->getComponent<Mob>(UPDATE_COMPONENT_NAME)),
		physics(tar->getComponent<PhysicsComponent>(PHYSICS_COMPONENT_NAME)),
		cameraPos(physics->getTranslation() - glm::vec3(0.0, 0.0, 10.0)),
		cameraLook(physics->getTranslation()),
		near(0.1f),
		far(10'000.0f),
		focalPoint(cameraPos),
		focalDistance(10.0f) {}

	/**
	 * Generates a view matrix for the camera.
	 * @return The view matrix.
	 */
	const glm::mat4 getView() const override {
		return glm::lookAt(cameraPos, cameraLook, glm::vec3(0.0, 1.0, 0.0));
	}

	/**
	 * Generates a projection matrix for the camera.
	 * @return The projection matrix.
	 */
	const glm::mat4 getProjection() const override { return projection; }

	/**
	 * Tells the camera to set its projection matrix. This is mainly called
	 * when the window's size changes.
	 */
	bool onEvent(const std::shared_ptr<const Event> event) override;

	/**
	 * Returns the near and far planes, for view culling.
	 * @return A pair containing the near plane in its first element and the
	 *     far plane in its second element.
	 */
	std::pair<float, float> getNearFar() const override { return {near, far}; }

	/**
	 * Returns the camera's field of view.
	 * @return The field of view.
	 */
	float getFOV() const override { return ExMath::PI / 4.0f; }

	/**
	 * Updates the camera's position, look position, and focal point.
	 */
	void update() override;

	/**
	 * Resets the camera's focal point based on the targeted object.
	 */
	void resetFocalPoint() {
		std::shared_ptr<MobState> state = mob->getState();
		focalPoint = physics->getTranslation() - focalDistance * glm::vec3(glm::mat4_cast(state->rotation) * glm::vec4(0.0, 0.0, -1.0, 0.0));
	}

	/**
	 * Returns the camera's focal point, which is primarily used for player movement.
	 * @return The focal point of the camera.
	 */
	glm::vec3 getFocalPoint() { return focalPoint; }

private:
	//Object the camera is watching.
	std::shared_ptr<Object> target;
	//The update (mob) component of the target object.
	std::shared_ptr<Mob> mob;
	//The physics for the target object.
	std::shared_ptr<PhysicsComponent> physics;

	//Position of the camera in the world.
	glm::vec3 cameraPos;
	//Where the camera is looking.
	glm::vec3 cameraLook;

	//The projection matrix.
	glm::mat4 projection;
	//Near plane distance.
	float near;
	//Far plane distance.
	float far;

	//Used to determine movement vectors and rotation.
	glm::vec3 focalPoint;
	//Target distance of the focal point from the object.
	float focalDistance;
};
