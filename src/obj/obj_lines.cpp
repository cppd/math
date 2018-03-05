/*
Copyright (C) 2017, 2018 Topological Manifold

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
template <size_t N>
class Lines final : public Obj<N>
{
        using typename Obj<N>::Facet;
        using typename Obj<N>::Point;
        using typename Obj<N>::Line;
        using typename Obj<N>::Material;
        using typename Obj<N>::Image;

        std::vector<Vector<N, float>> m_vertices;
        std::vector<Vector<N, float>> m_normals;
        std::vector<Vector<N - 1, float>> m_texcoords;
        std::vector<Facet> m_facets;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        Vector<N, float> m_center;
        float m_length;

        const std::vector<Vector<N, float>>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<Vector<N, float>>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Vector<N - 1, float>>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<Facet>& facets() const override
        {
                return m_facets;
        }
        const std::vector<Point>& points() const override
        {
                return m_points;
        }
        const std::vector<Line>& lines() const override
        {
                return m_lines;
        }
        const std::vector<Material>& materials() const override
        {
                return m_materials;
        }
        const std::vector<Image>& images() const override
        {
                return m_images;
        }
        Vector<N, float> center() const override
        {
                return m_center;
        }
        float length() const override
        {
                return m_length;
        }

        void create_obj(const std::vector<Vector<N, float>>& points, const std::vector<std::array<int, 2>>& lines)
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
                        Line l;

                        l.vertices[0] = index_map[line[0]];
                        l.vertices[1] = index_map[line[1]];

                        m_lines.push_back(std::move(l));
                }

                center_and_length(m_vertices, m_lines, &m_center, &m_length);
        }

public:
        Lines(const std::vector<Vector<N, float>>& points, const std::vector<std::array<int, 2>>& lines)
        {
                double start_time = time_in_seconds();

                create_obj(points, lines);

                LOG("Lines loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }
};
}

template <size_t N>
std::unique_ptr<Obj<N>> create_obj_for_lines(const std::vector<Vector<N, float>>& points,
                                             const std::vector<std::array<int, 2>>& lines)
{
        return std::make_unique<Lines<N>>(points, lines);
}

template std::unique_ptr<Obj<3>> create_obj_for_lines(const std::vector<Vector<3, float>>& points,
                                                      const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Obj<4>> create_obj_for_lines(const std::vector<Vector<4, float>>& points,
                                                      const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Obj<5>> create_obj_for_lines(const std::vector<Vector<5, float>>& points,
                                                      const std::vector<std::array<int, 2>>& lines);
template std::unique_ptr<Obj<6>> create_obj_for_lines(const std::vector<Vector<6, float>>& points,
                                                      const std::vector<std::array<int, 2>>& lines);
