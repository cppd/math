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

#include <src/painter/objects.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <memory>

namespace ns::painter::scenes
{
template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_cornell_box_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::array<int, N - 1>& screen_size,
        progress::Ratio* progress);
}
