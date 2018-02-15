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

#include "obj_surface.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "geometry/core/linear_algebra.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
template <size_t N>
Vector<N, double> face_normal(const std::vector<Vector<N, float>>& points, const std::array<int, N>& face)
{
        return normalize(ortho_nn<N, float, double>(points, face));
}

template <size_t N>
Vector<N, double> average_normal(const Vector<N, double>& original_normal, const std::vector<Vector<N, double>>& normals)
{
        Vector<N, double> sum(0);
        for (const Vector<N, double>& n : normals)
        {
                sum += (dot(n, original_normal) >= 0) ? n : -n;
        }
        return normalize(sum);
}

template <size_t N>
class SurfaceObj final : public Obj<N>
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

        void create_obj(const std::vector<Vector<N, float>>& points, const std::vector<Vector<N, double>>& normals,
                        const std::vector<std::array<int, N>>& facets)
        {
                if (facets.size() == 0)
                {
                        error("No facets for surface object");
                }

                std::unordered_map<int, std::vector<Vector<N, double>>> vertices;
                std::unordered_map<int, int> index_map;

                for (const std::array<int, N>& facet : facets)
                {
                        Vector<N, double> normal = face_normal(points, facet);
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
                        m_normals[idx] = to_vector<float>(average_normal(normals[v.first], v.second));

                        ++idx;
                }

                m_facets.reserve(facets.size());

                for (const std::array<int, N>& facet : facets)
                {
                        Facet obj_facet;

                        obj_facet.material = -1;
                        obj_facet.has_texcoord = false;
                        obj_facet.has_normal = true;

                        for (unsigned i = 0; i < N; ++i)
                        {
                                obj_facet.vertices[i] = index_map[facet[i]];
                                obj_facet.normals[i] = obj_facet.vertices[i];
                                obj_facet.texcoords[i] = -1;
                        }

                        m_facets.push_back(std::move(obj_facet));
                }

                center_and_length(m_vertices, m_facets, &m_center, &m_length);
        }

public:
        SurfaceObj(const std::vector<Vector<N, float>>& points, const std::vector<Vector<N, double>>& normals,
                   const std::vector<std::array<int, N>>& facets)
        {
                ASSERT(points.size() == normals.size());

                create_obj(points, normals, facets);
        }
};
}

template <size_t N>
std::unique_ptr<Obj<N>> create_obj_for_facets(const std::vector<Vector<N, float>>& points,
                                              const std::vector<Vector<N, double>>& normals,
                                              const std::vector<std::array<int, N>>& facets)

{
        return std::make_unique<SurfaceObj<N>>(points, normals, facets);
}

template std::unique_ptr<Obj<3>> create_obj_for_facets(const std::vector<Vector<3, float>>& points,
                                                       const std::vector<Vector<3, double>>& normals,
                                                       const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<Obj<4>> create_obj_for_facets(const std::vector<Vector<4, float>>& points,
                                                       const std::vector<Vector<4, double>>& normals,
                                                       const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<Obj<5>> create_obj_for_facets(const std::vector<Vector<5, float>>& points,
                                                       const std::vector<Vector<5, double>>& normals,
                                                       const std::vector<std::array<int, 5>>& facets);
