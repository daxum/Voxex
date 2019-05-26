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

#version 430 core

layout (location = 0) in vec3 posIn;
layout (location = 1) in uint normBlockPack;

out vec3 pos;
out vec3 normal;
out vec3 lightDir;
out vec2 posWorldSpace;
out vec2 texBase;

layout(binding = 0, std140) uniform ScreenData {
	mat4 projection;
	mat4 view;
} screen;

uniform mat4 modelView;

const vec4 normals[6] = vec4[6](
	vec4(0.0, 0.0, -1.0, 0.0),
	vec4(-1.0, 0.0, 0.0, 0.0),
	vec4(0.0, 0.0, 1.0, 0.0),
	vec4(1.0, 0.0, 0.0, 0.0),
	vec4(0.0, 1.0, 0.0, 0.0),
	vec4(0.0, -1.0, 0.0, 0.0)
);

const vec2 texLocs[2] = vec2[2](
	vec2(0.0, 0.0), //0
	vec2(0.5, 0.0)  //1
);

void main() {
	uint normalIndex = normBlockPack >> 16u;
	uint blockIndex = normBlockPack & 0xFFFFu;

	switch (normalIndex) {
		case 0: posWorldSpace = posIn.xy; break;
		case 1: posWorldSpace = posIn.yz; break;
		case 2: posWorldSpace = posIn.xy; break;
		case 3: posWorldSpace = posIn.yz; break;
		case 4: posWorldSpace = posIn.xz; break;
		case 5: posWorldSpace = posIn.xz; break;
	}

	pos = vec3(modelView * vec4(posIn, 1.0));
	gl_Position = screen.projection * modelView * vec4(posIn, 1.0);

	normal = vec3(modelView * normals[normalIndex]);
	lightDir = vec3(screen.view * vec4(normalize(vec3(-1.0, -1.0, 1.0)), 0.0));
	texBase = texLocs[blockIndex];
}
