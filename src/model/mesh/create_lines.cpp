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

#include "create_lines.h"

#include "position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ns::model::mesh
{
namespace
{
std::unordered_map<int, int> vertex_index_map(const std::vector<std::array<int, 2>>& lines)
{
        std::unordered_map<int, int> res;

        int idx = 0;
        for (const std::array<int, 2>& line : lines)
        {
                for (const int vertex_index : line)
                {
                        const auto [iter, inserted] = res.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(res.size()));

        return res;
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<numerical::Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        if (lines.empty())
        {
                error("No lines for line object");
        }

        const std::unordered_map<int, int> vertex_map = vertex_index_map(lines);

        auto mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertex_map.size());
        for (const auto& [old_index, new_index] : vertex_map)
        {
                mesh->vertices[new_index] = points[old_index];
        }

        mesh->lines.reserve(lines.size());
        for (const std::array<int, 2>& line : lines)
        {
                typename Mesh<N>::Line mesh_line;

                for (int i = 0; i < 2; ++i)
                {
                        const auto iter = vertex_map.find(line[i]);
                        ASSERT(iter != vertex_map.cend());
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
        const std::vector<numerical::Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = create_mesh(points, lines);

        LOG("Lines loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

#define TEMPLATE(N)                                                \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_lines( \
                const std::vector<numerical::Vector<(N), float>>&, const std::vector<std::array<int, 2>>&);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
