/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "convex_hull.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "com/vec.h"
#include "geometry/core/convex_hull.h"
#include "obj/alg/alg.h"

#include <unordered_map>

namespace
{
// template <size_t N>
// vec<N> average_normal(const std::vector<vec<N>>& normals)
//{
//        vec<N> v(0);
//        for (const vec<N>& n : normals)
//        {
//                v = v + n;
//        }
//        return normalize(v);
//}

template <size_t N>
class ConvexHullObj final : public Obj<N>
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

        void create_obj(const std::vector<Vector<N, float>>& points, const std::vector<ConvexHullFacet<N>>& facets)
        {
                if (facets.size() == 0)
                {
                        error("No facets for convex hull object");
                }

                std::unordered_map<int, std::vector<vec<N>>> vertex_map;
                std::unordered_map<int, int> index_map;

                for (const ConvexHullFacet<N>& facet : facets)
                {
                        vec<N> ortho = facet.ortho();
                        for (int v : facet.vertices())
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

                m_facets.reserve(facets.size());

                for (const ConvexHullFacet<N>& facet : facets)
                {
                        Facet obj_facet;

                        obj_facet.material = -1;
                        obj_facet.has_texcoord = false;
                        obj_facet.has_normal = false; // true;

                        for (unsigned i = 0; i < N; ++i)
                        {
                                obj_facet.vertices[i] = index_map[facet.vertices()[i]];
                                obj_facet.normals[i] = -1; // obj_facet.vertices[i];
                                obj_facet.texcoords[i] = -1;
                        }

                        m_facets.push_back(std::move(obj_facet));
                }

                center_and_length(m_vertices, m_facets, &m_center, &m_length);
        }

public:
        ConvexHullObj(const Obj<N>* obj, ProgressRatio* progress)
        {
                std::vector<Vector<N, float>> points;

                if (obj->facets().size() > 0)
                {
                        points = unique_facet_vertices(obj);
                }
                else if (obj->points().size() > 0)
                {
                        points = unique_point_vertices(obj);
                }
                else
                {
                        error("Faces or points not found for convex hull object");
                }

                std::vector<ConvexHullFacet<N>> facets;

                double start_time = time_in_seconds();

                compute_convex_hull(points, &facets, progress);

                LOG("Convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

                create_obj(points, facets);
        }
};
}

template <size_t N>
std::unique_ptr<Obj<N>> create_convex_hull_for_obj(const Obj<N>* obj, ProgressRatio* progress)
{
        return std::make_unique<ConvexHullObj<N>>(obj, progress);
}

template std::unique_ptr<Obj<3>> create_convex_hull_for_obj(const Obj<3>* obj, ProgressRatio* progress);
template std::unique_ptr<Obj<4>> create_convex_hull_for_obj(const Obj<4>* obj, ProgressRatio* progress);
template std::unique_ptr<Obj<5>> create_convex_hull_for_obj(const Obj<5>* obj, ProgressRatio* progress);
template std::unique_ptr<Obj<6>> create_convex_hull_for_obj(const Obj<6>* obj, ProgressRatio* progress);
