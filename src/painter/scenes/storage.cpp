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

#include "storage.h"

#include "scene.h"

#include <src/color/color.h>
#include <src/settings/instantiation.h>

namespace ns::painter::scenes
{
namespace
{
template <typename T>
[[nodiscard]] std::vector<typename T::pointer> to_pointers(const std::vector<T>& objects)
{
        std::vector<typename T::pointer> res;
        res.reserve(objects.size());
        for (const T& p : objects)
        {
                res.push_back(p.get());
        }
        return res;
}
}

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_storage_scene(
        const Color& background_light,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
        progress::Ratio* const progress)
{
        StorageScene<N, T, Color> res;

        res.projector = std::move(projector);
        res.light_sources = std::move(light_sources);
        res.shapes = std::move(shapes);

        res.scene = create_scene(
                background_light, clip_plane_equation, res.projector.get(), to_pointers(res.light_sources),
                to_pointers(res.shapes), progress);

        return res;
}

#define TEMPLATE(N, T, C)                                                                                       \
        template StorageScene<N, T, C> create_storage_scene(                                                    \
                const C&, const std::optional<Vector<(N) + 1, T>>&, std::unique_ptr<const Projector<(N), T>>&&, \
                std::vector<std::unique_ptr<const LightSource<(N), T, C>>>&&,                                   \
                std::vector<std::unique_ptr<const Shape<(N), T, C>>>&&, progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}