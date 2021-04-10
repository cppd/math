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

#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T>
struct PaintData final
{
        const Scene<N, T>& scene;
        const T ray_offset;
        const bool smooth_normal;

        PaintData(const Scene<N, T>& scene, T ray_offset, bool smooth_normal)
                : scene(scene), ray_offset(ray_offset), smooth_normal(smooth_normal)
        {
        }
};

struct TraceResult final
{
        std::optional<Color> color;
        int ray_count;
};

template <std::size_t N, typename T>
TraceResult trace_path(const PaintData<N, T>& paint_data, const Ray<N, T>& ray, RandomEngine<T>& random_engine);
}
