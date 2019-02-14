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
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec3 colorIn;

layout (location = 0) out vec3 pos;
layout (location = 1) out vec3 color;
layout (location = 2) out vec3 normal;
layout (location = 3) out vec3 lightDir;

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

void main() {
	gl_Position = screen.projection * chunk.modelView * vec4(posIn, 1.0);
	normal = vec3(chunk.modelView * vec4(normalIn, 0.0));
	color = colorIn;
	lightDir = (screen.view * vec4(normalize(vec3(-1.0, -1.0, 1.0)), 0.0)).xyz;
	pos = vec3(chunk.modelView * vec4(posIn, 1.0));
}
