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

#include "obj_surface.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
vec3f face_normal(const std::vector<vec3f>& points, const std::array<int, 3>& face)
{
        return normalize(cross(points[face[1]] - points[face[0]], points[face[2]] - points[face[0]]));
}

vec3f average_normal(const vec3f& original_normal, const std::vector<vec3f>& normals)
{
        vec3f sum(0);
        for (const vec3f& n : normals)
        {
                sum += (dot(n, original_normal) >= 0) ? n : -n;
        }
        return normalize(sum);
}

class SurfaceObj final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<face3> m_faces;
        std::vector<int> m_points;
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

        void create_obj(const std::vector<vec3f>& points, const std::vector<Vector<3, double>>& normals,
                        const std::vector<std::array<int, 3>>& facets)
        {
                if (facets.size() == 0)
                {
                        error("No facets for surface object");
                }

                std::unordered_map<int, std::vector<vec3f>> vertices;
                std::unordered_map<int, int> index_map;

                for (const std::array<int, 3>& facet : facets)
                {
                        vec3f normal = face_normal(points, facet);
                        for (int v : facet)
                        {
                                vertices[v].push_back(normal);
                        }
                }

                m_vertices.resize(vertices.size());
                m_normals.resize(vertices.size());

                int idx = 0;
                for (auto v : vertices)
                {
                        index_map[v.first] = idx;

                        m_vertices[idx] = points[v.first];
                        m_normals[idx] = average_normal(to_vector<float>(normals[v.first]), v.second);

                        ++idx;
                }

                m_faces.reserve(facets.size());

                for (const std::array<int, 3>& facet : facets)
                {
                        face3 face;

                        face.material = -1;
                        face.has_vt = false;
                        face.has_vn = true;

                        face.vertices[0].v = index_map[facet[0]];
                        face.vertices[1].v = index_map[facet[1]];
                        face.vertices[2].v = index_map[facet[2]];

                        face.vertices[0].vn = face.vertices[0].v;
                        face.vertices[1].vn = face.vertices[1].v;
                        face.vertices[2].vn = face.vertices[2].v;

                        face.vertices[0].vt = -1;
                        face.vertices[1].vt = -1;
                        face.vertices[2].vt = -1;

                        m_faces.push_back(std::move(face));
                }

                find_center_and_length(m_vertices, m_faces, &m_center, &m_length);
        }

public:
        SurfaceObj(const std::vector<vec3f>& points, const std::vector<Vector<3, double>>& normals,
                   const std::vector<std::array<int, 3>>& facets)
        {
                ASSERT(points.size() == normals.size());

                create_obj(points, normals, facets);
        }
};
}

std::unique_ptr<IObj> create_obj_for_facets(const std::vector<vec3f>& points, const std::vector<Vector<3, double>>& normals,
                                            const std::vector<std::array<int, 3>>& facets)
{
        return std::make_unique<SurfaceObj>(points, normals, facets);
}
