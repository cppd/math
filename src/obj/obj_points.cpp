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

#include "obj_points.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

namespace
{
class Points final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<Face> m_faces;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        vec3f m_center;
        float m_length;

        void read_points(std::vector<vec3f>&& points);

        const std::vector<vec3f>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Face>& faces() const override
        {
                return m_faces;
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
        vec3f center() const override
        {
                return m_center;
        }
        float length() const override
        {
                return m_length;
        }

public:
        Points(std::vector<vec3f>&& points);
};

void Points::read_points(std::vector<vec3f>&& points)
{
        m_vertices = std::move(points);

        if (m_vertices.size() == 0)
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

Points::Points(std::vector<vec3f>&& points)
{
        double start_time = get_time_seconds();

        read_points(std::move(points));

        LOG("Points loaded, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}
}

std::unique_ptr<IObj> create_obj_for_points(std::vector<vec3f>&& points)
{
        return std::make_unique<Points>(std::move(points));
}
