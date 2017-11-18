/*
Copyright (C) 2017 Topological Manifold

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

#include "obj_lines.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
class Lines final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<face3> m_faces;
        std::vector<int> m_points;
        std::vector<std::array<int, 2>> m_lines;
        std::vector<material> m_materials;
        std::vector<sf::Image> m_images;
        vec3f m_center;
        float m_length;

        const std::vector<vec3f>& get_vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& get_texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& get_normals() const override
        {
                return m_normals;
        }
        const std::vector<face3>& get_faces() const override
        {
                return m_faces;
        }
        const std::vector<int>& get_points() const override
        {
                return m_points;
        }
        const std::vector<std::array<int, 2>>& get_lines() const override
        {
                return m_lines;
        }
        const std::vector<material>& get_materials() const override
        {
                return m_materials;
        }
        const std::vector<sf::Image>& get_images() const override
        {
                return m_images;
        }
        vec3f get_center() const override
        {
                return m_center;
        }
        float get_length() const override
        {
                return m_length;
        }

        void create_obj(const std::vector<vec3f>& points, const std::vector<std::array<int, 2>>& lines)
        {
                if (lines.size() == 0)
                {
                        error("No lines for line object");
                }

                std::unordered_set<int> vertices;

                for (const std::array<int, 2>& line : lines)
                {
                        vertices.insert(line[0]);
                        vertices.insert(line[1]);
                }

                m_vertices.resize(vertices.size());

                std::unordered_map<int, int> index_map;

                int idx = 0;
                for (int v : vertices)
                {
                        ASSERT(v < static_cast<int>(points.size()));

                        index_map[v] = idx;
                        m_vertices[idx] = points[v];
                        ++idx;
                }

                m_lines.reserve(lines.size());

                for (const std::array<int, 2>& line : lines)
                {
                        m_lines.push_back({{index_map[line[0]], index_map[line[1]]}});
                }

                find_center_and_length(m_vertices, m_lines, &m_center, &m_length);
        }

public:
        Lines(const std::vector<vec3f>& points, const std::vector<std::array<int, 2>>& lines)
        {
                double start_time = get_time_seconds();

                create_obj(points, lines);

                LOG("Lines loaded, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
        }
};
}

std::unique_ptr<IObj> create_obj_for_lines(const std::vector<vec3f>& points, const std::vector<std::array<int, 2>>& lines)
{
        return std::make_unique<Lines>(points, lines);
}
