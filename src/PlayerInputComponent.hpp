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

#include "Components/AIComponent.hpp"

class PlayerInputComponent : public AIComponent{
public:
	static const std::string getName() { return AI_COMPONENT_NAME; }

	PlayerInputComponent() : AIComponent(true), setTarget(false), lastScreen(nullptr) {}

	void update(Screen* screen) override;

	bool onEvent(const InputHandler* handler, const std::shared_ptr<const InputEvent> event) override;

private:
	bool setTarget;
	//Temp hack, needs engine changes to pass screen to onEvent.
	Screen* lastScreen;
};