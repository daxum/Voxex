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

#include "Voxex.hpp"

int main(int argc, char** argv) {
	EngineConfig config = {};
	config.gameName = "Voxex";
	config.gameVersion = Engine::makeVersion(0, 0, 4);
	config.renderer.renderType = Voxex::USE_VULKAN ? Renderer::VULKAN : Renderer::OPEN_GL;
	config.renderer.windowWidth = 960;
	config.renderer.windowHeight = 540;
	config.renderer.windowTitle = "Voxex";
	config.renderer.deviceOverride = false;
	config.renderer.forceIndex = 1;
	config.renderer.validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
	config.timestep = 1000.0 / 60.0;
	config.physicsTimestep = 1.0f / 120.0f;
	config.frameReportFrequency = 5000;
	config.resourceBase = "";

	config.generalLog.type = LogType::STDOUT;
	config.generalLog.mask = INFO | WARN | ERROR | FATAL;

	config.rendererLog.type = LogType::STDOUT;
	config.rendererLog.mask = DEBUG | INFO | WARN | ERROR | FATAL;

	config.loaderLog.type = LogType::STDOUT;
	config.loaderLog.mask = INFO | WARN | ERROR | FATAL;

	config.modelLog.type = LogType::STDOUT;
	config.modelLog.mask = DEBUG | INFO | WARN | ERROR | FATAL;

	config.componentLog.type =  LogType::STDOUT;
	config.componentLog.mask = INFO | WARN | ERROR | FATAL;

	Engine engine(config);

	Voxex game;
	engine.run(game);

	return 0;
}
