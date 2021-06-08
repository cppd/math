/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <memory>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T>
std::unique_ptr<Scene<N, T>> create_storage_scene(
        const color::Color& background_light,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<const LightSource<N, T>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T>>>&& shapes);
}
