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

#include "obj.h"

#include "com/mat.h"

std::vector<int> get_unique_face_indices(const std::vector<IObj::face3>& faces);
std::vector<int> get_unique_point_indices(const std::vector<int>& points);
std::vector<int> get_unique_line_indices(const std::vector<std::array<int, 2>>& lines);

std::vector<vec3f> get_unique_face_vertices(const IObj* obj);
std::vector<vec3f> get_unique_point_vertices(const IObj* obj);

void find_min_max(const std::vector<vec3f>& vertices, const std::vector<int>& indices, vec3f* min, vec3f* max);
void find_min_max(const std::vector<vec3f>& vertices, vec3f* min, vec3f* max);

void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::face3>& faces, vec3f* center,
                            float* length);
void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<int>& points, vec3f* center, float* length);
void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<std::array<int, 2>>& lines, vec3f* center,
                            float* length);
void find_center_and_length(const std::vector<vec3f>& vertices, vec3f* center, float* length);

mat4 get_model_vertex_matrix(const IObj* obj, double size, const vec3& position);
