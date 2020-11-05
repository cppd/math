/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../objects.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/numerical/vec.h>

#include <memory>
#include <string>

namespace painter
{
template <typename T>
std::unique_ptr<const Scene<3, T>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        T size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up);

template <typename T>
std::unique_ptr<const Scene<3, T>> cornell_box_scene(
        int width,
        int height,
        std::unique_ptr<const Shape<3, T>>&& shape,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up);
}
