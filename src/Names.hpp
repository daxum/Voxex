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

//Buffers

const char* const CHUNK_VERTEX_BUFFER = "chnkvBfr";
const char* const CHUNK_INDEX_BUFFER = "chnkiBfr";
const char* const GENERIC_VERTEX_BUFFER = "gncvBfr";
const char* const GENERIC_INDEX_BUFFER = "gnciBfr";

//Vertex formats

const char* const CHUNK_FORMAT = "chnkFmt";
const char* const GENERIC_FORMAT = "gncFmt";

//Buffer elements

const char* const VERTEX_ELEMENT_PACKED_NORM_COLOR = "norCol";

//Shaders

const char* const CHUNK_SHADER = "chnkSdr";
const char* const BASIC_SHADER = "bscSdr";

//Uniform sets

const char* const CHUNK_SET = "chnkSet";
const char* const SCREEN_SET = "scrnSet";
const char* const BASIC_SET = "bscSet";

//Materials

const char* const CHUNK_MAT = "chnkMat";
const char* const PLAYER_MAT = "plrMat";

//Textures

const char* const TEST_TEX = "tstTex";

//Meshes

const char* const PLAYER_MESH = "plrMsh";
