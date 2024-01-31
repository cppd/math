/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/progress/progress.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace ns::painter::scenes
{
template <std::size_t N, typename T, typename Color>
struct StorageScene final
{
        std::unique_ptr<const Projector<N, T>> projector;
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources;
        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        std::unique_ptr<const Scene<N, T, Color>> scene;
};

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_storage_scene(
        const Color& background_light,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<LightSource<N, T, Color>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
        progress::Ratio* progress);
}
