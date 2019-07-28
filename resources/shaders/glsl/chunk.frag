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

out vec4 outColor;

in vec3 pos;
in vec3 color;
in vec3 normal;
in vec3 lightDir;
in vec2 posWorldSpace;
in vec2 texBase;

uniform sampler2D blockMapKd;

vec3 directionalBlinnPhong(vec2 texCoords) {
	vec3 norm = normalize(normal);
	vec3 position = -normalize(pos);

	vec3 colorKd = vec3(texture(blockMapKd, texCoords));

	vec3 ambient = colorKd;
	vec3 diffuse = colorKd * max(0, dot(lightDir, norm));
	vec3 specular = colorKd * pow(max(0, dot(normalize(lightDir + position), norm)), 10.0);

	return ambient + diffuse + specular;
}

void main() {
	const float blockTexSize = 0.5;

	vec2 texCoords = fract(posWorldSpace) * blockTexSize + texBase;
	outColor = vec4(directionalBlinnPhong(texCoords), 1.0);
}
