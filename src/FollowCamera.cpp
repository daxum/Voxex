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

#include "FollowCamera.hpp"
#include "Display/ScreenChangeEvent.hpp"
#include "Display/WindowSizeEvent.hpp"

bool FollowCamera::onEvent(const std::shared_ptr<const Event> event) {
	if (event->type == WindowSizeEvent::EVENT_TYPE || event->type == ScreenChangeEvent::EVENT_TYPE) {
		float width = Engine::instance->getWindowInterface().getWindowWidth();
		float height = Engine::instance->getWindowInterface().getWindowHeight();

		projection = glm::perspective(ExMath::PI / 4.0f, width / height, near, far);
	}

	return false;
}

void FollowCamera::update() {
	glm::vec3 targetPos = physics->getTranslation();

	cameraPos += -0.3f * (cameraPos - focalPoint);
	cameraPos.y = physics->getTranslation().y + 3.0;
	cameraLook += -0.3f * (cameraLook - physics->getTranslation());

	if (!mob->getTarget()) {
		//Update focal point based on object movement
		glm::vec3 focalVec = targetPos - focalPoint;
		focalVec.y = 0.0;
		focalPoint = targetPos - glm::normalize(focalVec) * focalDistance;
	}
	else {
		//Update focal point based on targeted object
		glm::vec3 trackedPos = mob->getTarget()->getTranslation();

		focalPoint = targetPos - focalDistance * glm::normalize(trackedPos - targetPos);
	}
}
