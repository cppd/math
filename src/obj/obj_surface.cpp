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

#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "geometry/core/vec_glm.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>

namespace
{
glm::vec3 face_normal(const std::vector<glm::vec3>& points, const std::array<int, 3>& face)
{
        return glm::normalize(glm::cross(points[face[1]] - points[face[0]], points[face[2]] - points[face[0]]));
}

glm::vec3 average_normal(const glm::vec3& original_normal, const std::vector<glm::vec3>& normals)
{
        glm::vec3 sum(0);
        for (const glm::vec3& n : normals)
        {
                sum += (glm::dot(n, original_normal) >= 0) ? n : -n;
        }
        return glm::normalize(sum);
}

class SurfaceObj final : public IObj
{
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec2> m_texcoords;
        std::vector<glm::vec3> m_normals;
        std::vector<face3> m_faces;
        std::vector<int> m_points;
        std::vector<material> m_materials;
        std::vector<sf::Image> m_images;
        glm::vec3 m_center;
        float m_length;

        const std::vector<glm::vec3>& get_vertices() const override
        {
                return m_vertices;
        }
        const std::vector<glm::vec2>& get_texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<glm::vec3>& get_normals() const override
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
        glm::vec3 get_center() const override
        {
                return m_center;
        }
        float get_length() const override
        {
                return m_length;
        }

        void create_obj(const std::vector<glm::vec3>& points, const std::vector<Vector<3, double>>& normals,
                        const std::vector<std::array<int, 3>>& facets)
        {
                if (facets.size() == 0)
                {
                        error("No facets for surface object");
                }

                std::unordered_map<int, std::vector<glm::vec3>> vertices;
                std::unordered_map<int, int> index_map;

                for (const std::array<int, 3>& facet : facets)
                {
                        glm::vec3 normal = face_normal(points, facet);
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
                        m_normals[idx] = average_normal(to_glm(normals[v.first]), v.second);

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
        SurfaceObj(const std::vector<glm::vec3>& points, const std::vector<Vector<3, double>>& normals,
                   const std::vector<std::array<int, 3>>& facets)
        {
                ASSERT(points.size() == normals.size());

                create_obj(points, normals, facets);
        }
};
}

std::unique_ptr<IObj> create_obj_for_facets(const std::vector<glm::vec3>& points, const std::vector<Vector<3, double>>& normals,
                                            const std::vector<std::array<int, 3>>& facets)
{
        return std::make_unique<SurfaceObj>(points, normals, facets);
}
