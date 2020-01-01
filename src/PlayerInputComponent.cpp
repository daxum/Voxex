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

#include "PlayerInputComponent.hpp"
#include "Components/PhysicsComponent.hpp"
#include "Components/PhysicsComponentManager.hpp"
#include "Mobs/MobState.hpp"
#include "Mobs/Mob.hpp"
#include "FollowCamera.hpp"

void PlayerInputComponent::update(Screen* screen) {
	lastScreen = screen;

	InputHandler& handler = screen->getInputHandler();
	std::shared_ptr<FollowCamera> camera = std::static_pointer_cast<FollowCamera>(screen->getCamera());
	std::shared_ptr<Mob> mob = lockParent()->getComponent<Mob>(UPDATE_COMPONENT_NAME);
	std::shared_ptr<MobState> state = mob->getState();
	std::shared_ptr<PhysicsComponent> physics = mob->getPhysics();
	std::shared_ptr<PhysicsComponentManager> world = mob->getPhysicsWorld(screen);

	glm::vec3 newVelocity(0.0, 0.0, 0.0);
	glm::vec3 focalXz = camera->getFocalPoint();
	focalXz.y = physics->getTranslation().y;
	glm::vec3 forward = glm::normalize(physics->getTranslation() - focalXz);
	glm::vec3 side = glm::cross(forward, glm::vec3(0.0, 1.0, 0.0));

	if (handler.isKeyPressed(Key::A)) {
		newVelocity -= side;
	}

	if (handler.isKeyPressed(Key::W)) {
		newVelocity += forward;
	}

	if (handler.isKeyPressed(Key::D)) {
		newVelocity += side;
	}

	if (handler.isKeyPressed(Key::S)) {
		newVelocity -= forward;
	}

	if (handler.isKeyPressed(Key::SPACE)) {
		mob->jump(newVelocity);
	}

	mob->move(newVelocity);

	if (setTarget) {
		RaytraceResult hitObject = world->raytraceUnderMouse();
		mob->setTarget(hitObject.hitComp);
		setTarget = false;
	}

	//For if you get stuck - debugging only
	if (handler.isKeyPressed(Key::SLASH)) {
		physics->applyImpulse({6.1, 5.0, 6.1});
	}
}

bool PlayerInputComponent::onEvent(const InputHandler* handler, const std::shared_ptr<const InputEvent> event) {
	if (event->type == EventType::MOUSE_CLICK) {
		std::shared_ptr<const MouseClickEvent> click = std::static_pointer_cast<const MouseClickEvent>(event);

		if (click->button == MouseButton::LEFT && click->action == MouseAction::PRESS) {
			lockParent()->getComponent<Mob>(UPDATE_COMPONENT_NAME)->attack();
			return true;
		}
		else if (click->button == MouseButton::RIGHT && click->action == MouseAction::PRESS) {
			//TODO: pass screen to events
			setTarget = true;
		}
	}
	else if (event->type == EventType::KEY) {
		std::shared_ptr<const KeyEvent> keyEvent = std::static_pointer_cast<const KeyEvent>(event);

		if (keyEvent->action == KeyAction::PRESS) {
			std::shared_ptr<FollowCamera> camera = std::static_pointer_cast<FollowCamera>(lastScreen->getCamera());

			switch (keyEvent->key) {
				case Key::LEFT_ALT: camera->resetFocalPoint(); return true;
				default: break;
			}
		}
	}

	return false;
}
