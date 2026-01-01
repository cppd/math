/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/random/pcg.h>
#include <src/numerical/ray.h>
#include <src/painter/objects.h>

#include <cstddef>
#include <optional>

namespace ns::painter::integrators::pt
{
template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Color> pt(const Scene<N, T, Color>& scene, const numerical::Ray<N, T>& ray, PCG& engine);
}
