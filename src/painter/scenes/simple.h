/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "storage.h"

#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/progress/progress.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>

namespace ns::painter::scenes
{
template <typename T, typename Color>
StorageScene<3, T, Color> create_simple_scene(
        std::unique_ptr<const Shape<3, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::optional<numerical::Vector<4, T>>& clip_plane_equation,
        std::type_identity_t<T> front_light_proportion,
        int screen_width,
        int screen_height,
        const numerical::Vector<3, T>& camera_up,
        const numerical::Vector<3, T>& camera_direction,
        const numerical::Vector<3, T>& light_direction,
        const numerical::Vector<3, T>& view_center,
        std::type_identity_t<T> view_width,
        progress::Ratio* progress);

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_simple_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        std::optional<std::type_identity_t<T>> clip_plane_position,
        std::type_identity_t<T> front_light_proportion,
        int max_screen_size,
        progress::Ratio* progress);
}
