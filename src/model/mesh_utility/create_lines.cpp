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

#include "create_lines.h"

#include "position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>

#include <unordered_map>

namespace ns::mesh
{
namespace
{
template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        if (lines.empty())
        {
                error("No lines for line object");
        }

        std::unordered_map<int, int> vertices;

        int idx = 0;
        for (const std::array<int, 2>& line : lines)
        {
                for (int vertex_index : line)
                {
                        auto [iter, inserted] = vertices.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());
        for (const auto& [old_index, new_index] : vertices)
        {
                mesh->vertices[new_index] = points[old_index];
        }

        mesh->lines.reserve(lines.size());
        for (const std::array<int, 2>& line : lines)
        {
                typename Mesh<N>::Line mesh_line;

                for (int i = 0; i < 2; ++i)
                {
                        auto iter = vertices.find(line[i]);
                        ASSERT(iter != vertices.cend());
                        mesh_line.vertices[i] = iter->second;
                }

                mesh->lines.push_back(std::move(mesh_line));
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_lines(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        TimePoint start_time = time();

        std::unique_ptr<Mesh<N>> mesh = create_mesh(points, lines);

        LOG("Lines loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

#define CREATE_MESH_FOR_LINES_INSTANTIATION(N)                     \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_lines( \
                const std::vector<Vector<(N), float>>&, const std::vector<std::array<int, 2>>&);

CREATE_MESH_FOR_LINES_INSTANTIATION(3)
CREATE_MESH_FOR_LINES_INSTANTIATION(4)
CREATE_MESH_FOR_LINES_INSTANTIATION(5)
CREATE_MESH_FOR_LINES_INSTANTIATION(6)
}
