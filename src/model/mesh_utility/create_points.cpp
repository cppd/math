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

#include "create_points.h"

#include "position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>

namespace ns::mesh
{
namespace
{
template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh(std::vector<Vector<N, float>>&& points)
{
        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices = std::move(points);

        if (mesh->vertices.empty())
        {
                error("No vertices found");
        }

        mesh->points.resize(mesh->vertices.size());
        for (unsigned i = 0; i < mesh->points.size(); ++i)
        {
                mesh->points[i].vertex = i;
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_points(std::vector<Vector<N, float>>&& points)
{
        TimePoint start_time = time();

        std::unique_ptr<Mesh<N>> mesh = create_mesh(std::move(points));

        LOG("Points loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> create_mesh_for_points(std::vector<Vector<3, float>>&& points);
template std::unique_ptr<Mesh<4>> create_mesh_for_points(std::vector<Vector<4, float>>&& points);
template std::unique_ptr<Mesh<5>> create_mesh_for_points(std::vector<Vector<5, float>>&& points);
template std::unique_ptr<Mesh<6>> create_mesh_for_points(std::vector<Vector<6, float>>&& points);
}
