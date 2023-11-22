/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "light_distribution.h"

#include "vertex/vertex.h"

#include "../../objects.h"

#include <optional>
#include <vector>

namespace ns::painter::integrators::bpt
{
template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const std::vector<vertex::Vertex<N, T, Color>>& light_path,
        const std::vector<vertex::Vertex<N, T, Color>>& camera_path,
        int s,
        int t,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine);
}
