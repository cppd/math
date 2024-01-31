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

#include "create_points.h"

#include "position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N, typename T>
std::unique_ptr<Mesh<N>> create_mesh(T&& points)
{
        if (points.empty())
        {
                error("No points for point object");
        }

        auto mesh = std::make_unique<Mesh<N>>();

        mesh->vertices = std::forward<T>(points);

        mesh->points.reserve(mesh->vertices.size());
        for (std::size_t i = 0; i < mesh->vertices.size(); ++i)
        {
                typename Mesh<N>::Point mesh_point;
                mesh_point.vertex = i;
                mesh->points.push_back(std::move(mesh_point));
        }

        set_center_and_length(mesh.get());

        return mesh;
}

template <std::size_t N, typename T>
std::unique_ptr<Mesh<N>> create_mesh_for_points_impl(T&& points)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = create_mesh<N>(std::forward<T>(points));

        LOG("Points loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_points(const std::vector<numerical::Vector<N, float>>& points)
{
        return create_mesh_for_points_impl<N>(points);
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_points(std::vector<numerical::Vector<N, float>>&& points)
{
        return create_mesh_for_points_impl<N>(std::move(points));
}

#define TEMPLATE(N)                                                                                                    \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_points(const std::vector<numerical::Vector<(N), float>>&); \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_points(std::vector<numerical::Vector<(N), float>>&&);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
