/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "create_lines.h"

#include "position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>

#include <unordered_map>
#include <unordered_set>

namespace mesh
{
namespace
{
template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        if (lines.empty())
        {
                error("No lines for line object");
        }

        std::unordered_set<int> vertices;

        for (const std::array<int, 2>& line : lines)
        {
                vertices.insert(line[0]);
                vertices.insert(line[1]);
        }

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());

        std::unordered_map<int, int> index_map;

        int idx = 0;
        for (int v : vertices)
        {
                ASSERT(v < static_cast<int>(points.size()));

                index_map[v] = idx;
                mesh->vertices[idx] = points[v];
                ++idx;
        }

        mesh->lines.reserve(lines.size());

        for (const std::array<int, 2>& line : lines)
        {
                typename Mesh<N>::Line l;

                l.vertices[0] = index_map[line[0]];
                l.vertices[1] = index_map[line[1]];

                mesh->lines.push_back(std::move(l));
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_lines(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        double start_time = time_in_seconds();

        std::unique_ptr<Mesh<N>> mesh = create_mesh(points, lines);

        LOG("Lines loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> create_mesh_for_lines(
        const std::vector<Vector<3, float>>& points,
        const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Mesh<4>> create_mesh_for_lines(
        const std::vector<Vector<4, float>>& points,
        const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Mesh<5>> create_mesh_for_lines(
        const std::vector<Vector<5, float>>& points,
        const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Mesh<6>> create_mesh_for_lines(
        const std::vector<Vector<6, float>>& points,
        const std::vector<std::array<int, 2>>& lines);
}
