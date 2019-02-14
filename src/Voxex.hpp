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

#include "GameInterface.hpp"

class Voxex : public GameInterface {
public:
	static constexpr bool USE_VULKAN = true;
	static const UniformSet chunkSet;

	void createRenderObjects(std::shared_ptr<RenderInitializer> renderInit);
	void loadTextures(std::shared_ptr<TextureLoader> loader) {}
	void loadModels(ModelLoader& loader) {}
	void loadShaders(std::shared_ptr<ShaderLoader> loader);
	void loadScreens(DisplayEngine& display);
};
