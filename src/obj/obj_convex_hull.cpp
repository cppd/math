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

#include "obj_convex_hull.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "com/vec.h"
#include "geometry/core/convex_hull.h"

#include <unordered_map>

namespace
{
// vec<3> average_normal(const std::vector<vec<3>>& normals)
//{
//        vec<3> v(0, 0, 0);
//        for (vec<3> n : normals)
//        {
//                v = v + n;
//        }
//        return normalize(v);
//}

class ConvexHullObj final : public IObj
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

        void create_obj(const std::vector<vec3f>& points, const std::vector<ConvexHullFacet<3>>& facets)
        {
                if (facets.size() == 0)
                {
                        error("No facets for convex hull object");
                }

                std::unordered_map<int, std::vector<vec<3>>> vertex_map;
                std::unordered_map<int, int> index_map;

                for (const ConvexHullFacet<3>& facet : facets)
                {
                        vec<3> ortho = facet.get_ortho();
                        for (int v : facet.get_vertices())
                        {
                                vertex_map[v].push_back(ortho);
                        }
                }

                m_vertices.resize(vertex_map.size());
                // m_normals.resize(vertex_map.size());

                int idx = 0;
                for (const auto& v : vertex_map)
                {
                        index_map[v.first] = idx;

                        m_vertices[idx] = points[v.first];
                        // m_normals[idx] = to_vector<float>(average_normal(v.second));

                        ++idx;
                }

                m_faces.reserve(facets.size());

                for (const ConvexHullFacet<3>& facet : facets)
                {
                        Face face;

                        face.material = -1;
                        face.has_texcoord = false;
                        face.has_normal = false; // true;

                        face.vertices[0] = index_map[facet.get_vertices()[0]];
                        face.vertices[1] = index_map[facet.get_vertices()[1]];
                        face.vertices[2] = index_map[facet.get_vertices()[2]];

                        face.normals[0] = -1; // face.vertices[0];
                        face.normals[1] = -1; // face.vertices[1];
                        face.normals[2] = -1; // face.vertices[2];

                        face.texcoords[0] = -1;
                        face.texcoords[1] = -1;
                        face.texcoords[2] = -1;

                        m_faces.push_back(std::move(face));
                }

                center_and_length(m_vertices, m_faces, &m_center, &m_length);
        }

public:
        ConvexHullObj(const IObj* obj, ProgressRatio* progress)
        {
                std::vector<vec3f> points;

                if (obj->faces().size() > 0)
                {
                        points = unique_face_vertices(obj);
                }
                else if (obj->points().size() > 0)
                {
                        points = unique_point_vertices(obj);
                }
                else
                {
                        error("Faces or points not found for convex hull object");
                }

                std::vector<ConvexHullFacet<3>> facets;

                double start_time = time_in_seconds();

                compute_convex_hull(points, &facets, progress);

                LOG("Convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

                create_obj(points, facets);
        }
};
}

std::unique_ptr<IObj> create_convex_hull_for_obj(const IObj* obj, ProgressRatio* progress)
{
        return std::make_unique<ConvexHullObj>(obj, progress);
}
