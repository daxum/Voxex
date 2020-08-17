/******************************************************************************
 * SGIS - A simple game involving squares
 * Copyright (C) 2017
 *
 * Forked 2019/02/12
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

#include <glm/glm.hpp>

#include "Display/Camera.hpp"
#include "Engine.hpp"
#include "ExtraMath.hpp"
#include "SplineAnimation.hpp"
#include "Display/ScreenChangeEvent.hpp"
#include "Display/WindowSizeEvent.hpp"

class AnimatedCamera : public Camera {
public:
	AnimatedCamera(const std::vector<std::pair<glm::vec3, glm::quat>>& frames, float time) :
		anim(frames, time),
		currentTime(0),
		near(0.1f),
		far(6000.0f) {

	}

	const glm::mat4 getView() const override {
		const std::pair<glm::vec3, glm::quat> loc = anim.getLocation((float)currentTime);
		return glm::lookAt(loc.first, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	}

	const glm::mat4 getProjection() const override {
		return projection;
	}

	bool onEvent(const std::shared_ptr<const Event> event) override {
		if (event->type == WindowSizeEvent::EVENT_TYPE || event->type == ScreenChangeEvent::EVENT_TYPE) {
			float width = Engine::instance->getWindowInterface().getWindowWidth();
			float height = Engine::instance->getWindowInterface().getWindowHeight();

			projection = glm::perspective(ExMath::PI / 4.0f, width / height, near, far);
		}

		return false;
	}

	std::pair<float, float> getNearFar() const override {
		return {near, far};
	}

	float getFOV() const override { return ExMath::PI / 4.0f; }

	void update() override {
		currentTime++;
	}

private:
	//The animation to follow
	SplineAnimation anim;
	//number of ticks for the camera
	size_t currentTime;

	//Near plane, far plane, projection matrix.
	float near;
	float far;
	glm::mat4 projection;
};
