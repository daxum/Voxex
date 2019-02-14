/******************************************************************************
 * SGIS - A simple game involving squares
 * Copyright (C) 2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#pragma once

#include <glm/glm.hpp>

#include "Object.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "ExtraMath.hpp"
#include "InputListener.hpp"

//Follows an object around at a fixed height.
class SquareCamera : public Camera {
public:
	/**
	 * Creates a camera at the given position looking in the specific direction
	 * @param parent The parent screen.
	 * @param startPos The camera's starting position
	 * @param startLook The place where the camera is looking
	 * @param startup Which direction is "up" - should almost always be (0, 1, 0)
	 */
	SquareCamera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 startLook = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 startUp = glm::vec3(0.0f, 1.0f, 0.0f));

	/**
	 * Calculates a view matrix to use in rendering
	 * @return the view matrix
	 */
	const glm::mat4 getView() const override { return glm::lookAt(pos, look, up); }

	/**
	 * Gets the projection matrix.
	 */
	const glm::mat4 getProjection() const override { return projection; }

	/**
	 * Sets the projection matrix.
	 */
	void setProjection() override;

	/**
	 * Gets the near and far planes.
	 */
	std::pair<float, float> getNearFar() const override { return {near, far}; }

	/**
	 * Gets field of view.
	 */
	float getFOV() const override { return ExMath::PI / 4.0f; }

	/**
	 * Updates the camera. This currently moves it to track the object it is following.
	 */
	void update() override;

	/**
	 * Sets the object for the camera to track.
	 */
	void setTarget(std::shared_ptr<Object> object);

private:
	//These three vectors define a camera - pos is position,
	//look is where the camera is looking, up is the up vector.
	glm::vec3 pos;
	glm::vec3 look;
	glm::vec3 up;

	//The object to follow.
	std::shared_ptr<Object> target;

	//The velocity of the camera.
	glm::vec3 velocity;

	//Near plane, far plane, and projection matrix.
	float near;
	float far;
	glm::mat4 projection;
};

