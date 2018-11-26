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

#include "com/color/color.h"
#include "painter/objects.h"
#include "painter/shapes/mesh.h"

#include <memory>

std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(int width, int height, const std::string& obj_file_name,
                                                                 double size, const Color& default_color, double diffuse,
                                                                 const vec3& camera_direction, const vec3& camera_up);

std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(int width, int height,
                                                                 const std::shared_ptr<const Mesh<3, double>>& mesh, double size,
                                                                 const Color& default_color, double diffuse,
                                                                 const vec3& camera_direction, const vec3& camera_up);
