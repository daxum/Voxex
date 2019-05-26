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

#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 posIn;
layout (location = 1) in uint normBlockPack;

//Position in camera space, for lighting
layout (location = 0) out vec3 posCamSpace;
//Normal
layout (location = 1) out vec3 normal;
//Currently hard-coded light direction
layout (location = 2) out vec3 lightDir;
//Position in world space, for getting per-pixel texture coords
layout (location = 3) out vec2 posWorldSpace;
//Top left corner of the tex coords for the block type
layout (location = 4) out vec2 texCoords;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (set = 0, binding = 0, std140) uniform ScreenData {
	layout (offset = 0) mat4 projection;
	layout (offset = 64) mat4 view;
} screen;

layout (push_constant, std430) uniform ChunkData {
	layout (offset = 0) mat4 modelView;
} chunk;

const vec4 normals[6] = vec4[6](
	vec4(0.0, 0.0, -1.0, 0.0),
	vec4(-1.0, 0.0, 0.0, 0.0),
	vec4(0.0, 0.0, 1.0, 0.0),
	vec4(1.0, 0.0, 0.0, 0.0),
	vec4(0.0, 1.0, 0.0, 0.0),
	vec4(0.0, -1.0, 0.0, 0.0)
);

//TODO: move to buffer
const vec2 texLocs[2] = vec2[2](
	vec2(0.0, 0.0), //0
	vec2(0.5, 0.0)  //1
);

void main() {
	uint normalIndex = normBlockPack >> 16u;
	uint blockIndex = normBlockPack & 0xFFFF;

	switch (normalIndex) {
		case 0: posWorldSpace = posIn.xy; break;
		case 1: posWorldSpace = posIn.yz; break;
		case 2: posWorldSpace = posIn.xy; break;
		case 3: posWorldSpace = posIn.yz; break;
		case 4: posWorldSpace = posIn.xz; break;
		case 5: posWorldSpace = posIn.xz; break;
	}

	posCamSpace = vec3(chunk.modelView * vec4(posIn, 1.0));
	gl_Position = screen.projection * chunk.modelView * vec4(posIn, 1.0);

	normal = vec3(chunk.modelView * normals[normalIndex]);
	lightDir = vec3(screen.view * vec4(normalize(vec3(-1.0, -1.0, 1.0)), 0.0));
	texCoords = texLocs[blockIndex];
}
