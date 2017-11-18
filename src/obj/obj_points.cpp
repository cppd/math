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

#include "obj_points.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

#include <numeric>

namespace
{
class Points final : public IObj
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

        void read_points(std::vector<vec3f>&& points);

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
        std::iota(m_points.begin(), m_points.end(), 0);

        find_center_and_length(m_vertices, m_points, &m_center, &m_length);
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
