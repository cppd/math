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
#include "geom/surface_cocone.h"
#include "geom/vec_glm.h"

#include <chrono>
#include <random>
#include <unordered_map>
#include <unordered_set>

namespace
{
class SurfaceObj final : public IObj
{
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec2> m_texcoords;
        std::vector<glm::vec3> m_normals;
        std::vector<face3> m_faces;
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

        void create_obj(const std::vector<Vector<3, float>>& points, const std::vector<Vector<3, double>>& normals,
                        const std::vector<std::array<int, 3>>& facets)
        {
                std::unordered_set<int> vertex_set;
                std::unordered_map<int, int> index_map;

                for (const std::array<int, 3>& facet : facets)
                {
                        for (int v : facet)
                        {
                                vertex_set.insert(v);
                        }
                }

                m_vertices.resize(vertex_set.size());
                m_normals.resize(vertex_set.size());

                int idx = 0;
                for (int v : vertex_set)
                {
                        index_map[v] = idx;

                        m_vertices[idx] = to_glm(points[v]);
                        m_normals[idx] = to_glm(normals[v]);

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

#if 0
        std::vector<Vector<3, float>> generate_random_data(int count)
        {
                std::vector<Vector<3, float>> points;

                std::mt19937_64 gen(count);
                std::uniform_real_distribution<double> urd(-1.0, 1.0);

                points.resize(count);

                for (int v_i = 0; v_i < count; ++v_i)
                {
                        vec<3> v;
                        do
                        {
                                for (unsigned i = 0; i < 3; ++i)
                                {
                                        v[i] = urd(gen);
                                }
                        } while (length(v) > 1);

                        points[v_i] = to_vec<float>(normalize(v));
                        points[v_i][0] *= 1.1;

                        // points[v_i][2] = std::fabs(points[v_i][2]);
                }

                return points;
        }
#endif

public:
        SurfaceObj(ConvexHullComputationType ct, const IObj* obj, ProgressRatio* progress)
        {
                // std::vector<Vector<3, float>> points = generate_random_data(1000);
                std::vector<Vector<3, float>> points = to_vector<float>(obj->get_vertices());

                std::vector<vec<3>> normals;
                std::vector<std::array<int, 3>> facets;

                double start_time = get_time_seconds();

                std::unique_ptr<ISurfaceReconstructor<3>> sr = create_surface_reconstructor(ct, points, progress);
                sr->cocone(&normals, &facets, progress);
                // sr->bound_cocone(0.3, 0.14, &normals, &facets, progress);

                LOG("Surface reconstructed, " + to_string(get_time_seconds() - start_time, 5) + " s");

                create_obj(points, normals, facets);
        }

        SurfaceObj(const std::vector<Vector<3, float>>& points, const std::vector<Vector<3, double>>& normals,
                   const std::vector<std::array<int, 3>>& facets)
        {
                ASSERT(points.size() == normals.size());

                create_obj(points, normals, facets);
        }
};
}

std::unique_ptr<IObj> create_surface_for_obj(ConvexHullComputationType ct, const IObj* obj, ProgressRatio* progress)
{
        return std::make_unique<SurfaceObj>(ct, obj, progress);
}

std::unique_ptr<IObj> create_obj_for_facets(const std::vector<Vector<3, float>>& points,
                                            const std::vector<Vector<3, double>>& normals,
                                            const std::vector<std::array<int, 3>>& facets)
{
        return std::make_unique<SurfaceObj>(points, normals, facets);
}
