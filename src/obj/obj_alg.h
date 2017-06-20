/*
Copyright (C) 2017 Topological Manifold

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
#ifndef OBJ_ALG_H
#define OBJ_ALG_H

#include "obj.h"

std::vector<int> get_unique_face_indices(const std::vector<IObj::face3> faces);

std::vector<glm::vec3> get_unique_face_vertices(const IObj* obj);

void find_min_max(const std::vector<glm::vec3>& vertices, const std::vector<int>& indices, glm::vec3* min, glm::vec3* max);

void find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<IObj::face3> faces, glm::vec3* center,
                            float* length);

#endif
