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

#version 410 core

layout (location = 0) in vec3 posIn;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec3 colorIn;

out vec3 pos;
out vec3 color;
out vec3 normal;
out vec3 lightDir;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 modelView;

void main() {
	gl_Position = projection * modelView * vec4(posIn, 1.0);
	normal = vec3(modelView * vec4(normalIn, 0.0));
	color = colorIn;
	lightDir = (view * vec4(normalize(vec3(-1.0, -1.0, 1.0)), 0.0)).xyz;
	pos = vec3(modelView * vec4(posIn, 1.0));
}
