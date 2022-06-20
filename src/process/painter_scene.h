/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/painter/shapes/shape.h>
#include <src/progress/progress.h>

#include <memory>
#include <optional>

namespace ns::process
{
template <typename T, typename Color>
std::unique_ptr<const painter::Scene<3, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<3, T, Color>>&& shape,
        const Vector<3, T>& camera_up,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& light_direction,
        const Vector<3, T>& view_center,
        std::type_identity_t<T> view_width,
        const std::optional<Vector<4, T>>& clip_plane_equation,
        int width,
        int height,
        bool cornell_box,
        const Color& light,
        const Color& background_light,
        progress::Ratio* progress);

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const painter::Scene<N, T, Color>> create_painter_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>>&& shape,
        int max_screen_size,
        bool cornell_box,
        const Color& light,
        const Color& background_light,
        std::optional<std::type_identity_t<T>> clip_plane_position,
        progress::Ratio* progress);
}
