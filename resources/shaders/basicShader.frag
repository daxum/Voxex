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

out vec4 outColor;

in vec3 pos;
in vec3 color;
in vec3 normal;

uniform vec3 lightDir;

vec3 directionalBlinnPhong() {
	vec3 norm = normalize(normal);
	vec3 position = -normalize(pos);

	vec3 ambient = color;
	vec3 diffuse = color * max(0, dot(lightDir, norm));
	vec3 specular = color * pow(max(0, dot(normalize(lightDir + position), norm)), 200.0);

	return ambient + diffuse + specular;
}

void main() {
	outColor = vec4(directionalBlinnPhong(), 1.0);
}
