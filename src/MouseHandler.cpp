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

#include "MouseHandler.hpp"
#include "Components/RenderComponent.hpp"

void MouseHandler::update(Screen* screen) {
	std::shared_ptr<PhysicsComponentManager> world = screen->getManager<PhysicsComponentManager>(PHYSICS_COMPONENT_NAME);

	RaytraceResult hitObject = world->raytraceUnderMouse();
	std::shared_ptr<Chunk> hitChunk = chunkLoader->getChunk(hitObject.hitPos);

	if (hitObject.hitComp && hitChunk && hitObject.hitComp->getParent() == hitChunk->getObject()) {
		lockParent()->getComponent<RenderComponent>()->setHidden(false);

		glm::vec3 normal = hitObject.hitNormal;
		//Has all components as 1 except the original normal component
		glm::vec3 invNormal = glm::abs(glm::abs(normal) - glm::vec3(1, 1, 1));

		glm::vec3 normalComp = hitObject.hitPos - normal / 2.0f;
		glm::vec3 otherComp = glm::trunc(hitObject.hitPos) + glm::sign(hitObject.hitPos) / 2.0f;

		boxPos = normalComp * glm::abs(normal) + otherComp * invNormal;
	}
	else {
		lockParent()->getComponent<RenderComponent>()->setHidden(true);
	}
}
