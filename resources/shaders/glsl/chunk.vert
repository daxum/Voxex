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
layout (location = 1) in uint normColPack;

out vec3 pos;
out vec3 color;
out vec3 normal;
out vec3 lightDir;

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

const vec3 colors[20] = vec3[20](
	vec3(0.200, 0.200, 0.200), //0
	vec3(0.430, 0.366, 0.075), //1
	vec3(0.100, 0.900, 0.150), //2
	vec3(0.900, 0.010, 0.200), //3
	vec3(0.010, 0.300, 0.950), //4
	vec3(0.500, 0.500, 0.300), //5
	vec3(0.300, 0.750, 0.800), //6
	vec3(1.000, 1.000, 1.000), //7
	vec3(1.000, 0.000, 0.000), //8
	vec3(0.000, 1.000, 0.000), //9
	vec3(0.000, 0.000, 1.000), //10
	vec3(0.100, 0.100, 0.100), //11
	vec3(0.200, 0.200, 0.200), //12
	vec3(0.300, 0.300, 0.300), //13
	vec3(0.400, 0.400, 0.400), //14
	vec3(0.500, 0.500, 0.500), //15
	vec3(0.600, 0.600, 0.600), //16
	vec3(0.700, 0.700, 0.700), //17
	vec3(0.800, 0.800, 0.800), //18
	vec3(0.900, 0.900, 0.900) //19
);

void main() {
	uint normalIndx = normColPack >> 16u;
	uint colorIndx = normColPack & 0xFFFF;

	gl_Position = screen.projection * modelView * vec4(posIn, 1.0);
	normal = vec3(modelView * normals[normalIndx]);
	color = colors[colorIndx];
	lightDir = (screen.view * vec4(normalize(vec3(-1.0, -1.0, 1.0)), 0.0)).xyz;
	pos = vec3(modelView * vec4(posIn, 1.0));
}
