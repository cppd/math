/*
Copyright (C) 2017, 2018 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "com/vec.h"

// GLSL имеет размер float == 4
constexpr int GLSL_VEC3_ALIGN = 4 * sizeof(float); // для vec3 выравнивание по 4 * N

static_assert(sizeof(vec2f) == 2 * sizeof(float));
static_assert(sizeof(vec3f) == 3 * sizeof(float));
static_assert(sizeof(vec4f) == 4 * sizeof(float));
