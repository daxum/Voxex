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
layout (location = 1) in vec3 normIn;
layout (location = 2) in vec2 texIn;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 pos;
layout(location = 1) out vec3 norm;
layout(location = 2) out vec2 tex;

layout(set = 0, binding = 0, std140) uniform ScreenData {
	mat4 projection;
	mat4 view;
} screen;

layout(push_constant, std430) uniform ObjectData {
	layout(offset = 0) mat4 modelView;
} object;

void main() {
	vec4 posCameraSpace = object.modelView * vec4(posIn, 1.0);

	pos = posCameraSpace.xyz;
	norm = (object.modelView * vec4(normIn, 0.0)).xyz;
	tex = texIn;

	gl_Position = screen.projection * posCameraSpace;
}
