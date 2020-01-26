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

#include "points.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "obj/alg/alg.h"

namespace
{
template <size_t N>
class Points final : public Obj<N>
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

        void read_points(std::vector<Vector<N, float>>&& points);

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

public:
        Points(std::vector<Vector<N, float>>&& points);
};

template <size_t N>
void Points<N>::read_points(std::vector<Vector<N, float>>&& points)
{
        m_vertices = std::move(points);

        if (m_vertices.empty())
        {
                error("No vertices found");
        }

        m_points.resize(m_vertices.size());
        for (unsigned i = 0; i < m_points.size(); ++i)
        {
                m_points[i].vertex = i;
        }

        center_and_length(m_vertices, m_points, &m_center, &m_length);
}

template <size_t N>
Points<N>::Points(std::vector<Vector<N, float>>&& points)
{
        double start_time = time_in_seconds();

        read_points(std::move(points));

        LOG("Points loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}
}

template <size_t N>
std::unique_ptr<Obj<N>> create_obj_for_points(std::vector<Vector<N, float>>&& points)
{
        return std::make_unique<Points<N>>(std::move(points));
}

template std::unique_ptr<Obj<3>> create_obj_for_points(std::vector<Vector<3, float>>&& points);
template std::unique_ptr<Obj<4>> create_obj_for_points(std::vector<Vector<4, float>>&& points);
template std::unique_ptr<Obj<5>> create_obj_for_points(std::vector<Vector<5, float>>&& points);
template std::unique_ptr<Obj<6>> create_obj_for_points(std::vector<Vector<6, float>>&& points);
