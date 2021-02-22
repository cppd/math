/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "sphere_create.h"

#include "../core/convex_hull.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/sort.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ns::geometry
{
namespace
{
template <std::size_t N, typename T>
bool equal_distances(const std::array<Vector<N, T>, N>& vertices)
{
        if (vertices.size() < 2)
        {
                return true;
        }
        const T d = (vertices[0] - vertices[1]).norm();
        for (unsigned i = 0; i < vertices.size(); ++i)
        {
                for (unsigned j = i + 1; j < vertices.size(); ++j)
                {
                        if (std::abs(d - (vertices[j] - vertices[i]).norm()) > T(1e-3))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N, typename T>
bool equal_distance_from_origin(const std::array<Vector<N, T>, N>& vertices)
{
        if (vertices.empty())
        {
                return true;
        }
        const T d = vertices[0].norm();
        for (unsigned i = 1; i < vertices.size(); ++i)
        {
                if (std::abs(d - vertices[i].norm()) > T(1e-3))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
struct Facet final
{
        std::array<Vector<N, T>, N> vertices;

        Facet()
        {
        }

        template <typename... Ts>
        explicit Facet(Ts... vertices) : vertices{vertices...}
        {
                static_assert(sizeof...(Ts) == N);
        }
};

//template <std::size_t N>
//class FacetSubdivision final
//{
//        static_assert(N >= 4);
//
//        static constexpr std::size_t MIDPOINT_COUNT = (N * (N - 1)) / 2;
//
//        std::vector<std::array<int, N>> m_facets;
//
//        template <std::size_t M>
//        static bool on_plane(const std::array<int, M>& object_indices, const std::vector<Vector<N, float>>& vertices)
//        {
//                Vector<N, float> sum(0);
//                for (unsigned i : object_indices)
//                {
//                        ASSERT(i < vertices.size());
//                        sum += vertices[i];
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        if (sum[i] == 0)
//                        {
//                                return true;
//                        }
//                }
//                return false;
//        }
//
//        template <std::size_t M>
//        static std::string vertex_indices_to_string(
//                std::array<int, M> vertex_indices,
//                const std::vector<std::string>& vertex_names)
//        {
//                std::sort(vertex_indices.begin(), vertex_indices.end());
//                std::string s;
//                for (unsigned v : vertex_indices)
//                {
//                        if (!s.empty())
//                        {
//                                s += ", ";
//                        }
//                        s += vertex_names[v];
//                }
//                return s;
//        }
//
//        static void check(
//                std::vector<std::array<int, N>> facets,
//                const std::vector<Vector<N, float>>& vertices,
//                const std::vector<std::string>& vertex_names)
//        {
//                ASSERT(vertices.size() == N + MIDPOINT_COUNT);
//
//                for (std::array<int, N>& facet : facets)
//                {
//                        std::sort(facet.begin(), facet.end());
//                }
//                std::sort(facets.begin(), facets.end());
//
//                std::map<std::array<int, N - 1>, int> boundary_ridges;
//                std::map<std::array<int, N - 1>, int> internal_ridges;
//                std::map<std::array<int, N - 1>, int> ridges;
//
//                for (const std::array<int, N>& facet : facets)
//                {
//                        for (unsigned r = 0; r < N; ++r)
//                        {
//                                ASSERT(facet[r] < static_cast<int>(vertices.size()));
//                                std::array<int, N - 1> ridge = sort(del_elem(facet, r));
//                                if (on_plane(ridge, vertices))
//                                {
//                                        ++boundary_ridges[ridge];
//                                }
//                                else
//                                {
//                                        ++internal_ridges[ridge];
//                                }
//                                ++ridges[ridge];
//                                ASSERT(ridges[ridge] <= 2);
//                        }
//                }
//
//                ASSERT(boundary_ridges.size() % N == 0);
//                ASSERT(boundary_ridges.size() > N * (N - 1));
//                ASSERT((boundary_ridges.size() - N * (N - 1)) % N == 0);
//
//                std::ostringstream oss;
//
//                oss << "Facet subdivisions " << facets.size();
//                for (const std::array<int, N>& facet : facets)
//                {
//                        oss << '\n' << vertex_indices_to_string(facet, vertex_names);
//                }
//
//                oss << '\n' << "Boundary ridges " << boundary_ridges.size();
//                for (const auto& [ridge, count] : boundary_ridges)
//                {
//                        ASSERT(count == 1);
//                        oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                }
//
//                oss << '\n' << "Internal ridges " << internal_ridges.size();
//                for (const auto& [ridge, count] : internal_ridges)
//                {
//                        ASSERT(count == 2);
//                        oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                }
//
//                LOG(oss.str());
//        }
//
//public:
//        FacetSubdivision()
//        {
//                std::vector<Vector<N, float>> vertices;
//                std::vector<std::string> vertex_names;
//
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        vertices.emplace_back(0);
//                        vertices.back()[i] = 1;
//                        vertex_names.push_back(to_string(i));
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        for (unsigned j = i + 1; j < N; ++j)
//                        {
//                                vertices.push_back((vertices[i] + vertices[j]).normalized());
//                                vertex_names.push_back(to_string(i) + (N <= 10 ? "" : "_") + to_string(j));
//                        }
//                }
//
//                vertices.emplace_back(0);
//
//                ASSERT(vertices.size() == N + MIDPOINT_COUNT + 1);
//
//                std::vector<ConvexHullFacet<N>> facets;
//                ProgressRatio progress(nullptr);
//
//                compute_convex_hull(vertices, &facets, &progress);
//
//                for (const ConvexHullFacet<N>& facet : facets)
//                {
//                        if (!on_plane(facet.vertices(), vertices))
//                        {
//                                m_facets.push_back(facet.vertices());
//                        }
//                }
//
//                vertices.resize(vertices.size() - 1);
//
//                check(m_facets, vertices, vertex_names);
//        }
//
//        std::size_t facet_count() const
//        {
//                return m_facets.size();
//        }
//
//        template <typename T>
//        void divide(const Facet<N, T>& facet, std::vector<Facet<N, T>>* facets) const
//        {
//                std::array<Vector<N, T>, N + MIDPOINT_COUNT> points;
//
//                unsigned index = 0;
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        points[index++] = facet.vertices[i];
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        for (unsigned j = i + 1; j < N; ++j)
//                        {
//                                points[index++] = (points[i] + points[j]).normalized();
//                        }
//                }
//                ASSERT(index == points.size());
//                for (const std::array<int, N>& indices : m_facets)
//                {
//                        Facet<N, T>& f = facets->emplace_back();
//                        for (unsigned i = 0; i < N; ++i)
//                        {
//                                ASSERT(indices[i] < static_cast<int>(points.size()));
//                                f.vertices[i] = points[indices[i]];
//                        }
//                }
//        }
//};

template <typename T>
std::vector<Facet<3, T>> divide_facets(unsigned min_facet_count, std::vector<Facet<3, T>> facets)
{
        ASSERT(facets.size() >= (1 << 3));

        while (facets.size() < min_facet_count)
        {
                std::vector<Facet<3, T>> tmp;
                tmp.reserve(4 * facets.size());

                for (const Facet<3, T>& facet : facets)
                {
                        const Vector<3, T>& v0 = facet.vertices[0];
                        const Vector<3, T>& v1 = facet.vertices[1];
                        const Vector<3, T>& v2 = facet.vertices[2];
                        Vector<3, T> p01 = (v0 + v1).normalized();
                        Vector<3, T> p12 = (v1 + v2).normalized();
                        Vector<3, T> p20 = (v2 + v0).normalized();
                        tmp.emplace_back(v0, p01, p20);
                        tmp.emplace_back(v1, p12, p01);
                        tmp.emplace_back(v2, p20, p12);
                        tmp.emplace_back(p01, p12, p20);
                }

                facets = std::move(tmp);
        }

        return facets;
}

template <std::size_t N, typename T>
std::enable_if_t<N >= 4, std::vector<Facet<N, T>>> divide_facets(
        unsigned min_facet_count,
        std::vector<Facet<N, T>> facets)
{
        ASSERT(facets.size() >= (1 << N));

        std::unordered_set<Vector<N, float>> vertex_set;
        std::vector<Vector<N, float>> vertices;

        unsigned subdivision_count = 0;

        while (facets.size() < min_facet_count)
        {
                std::vector<Facet<N, T>> tmp;
                tmp.reserve(subdivision_count * facets.size());

                vertex_set.clear();
                for (const Facet<N, T>& facet : facets)
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                vertex_set.insert(to_vector<float>(facet.vertices[i]));
                        }
                        for (unsigned i = 0; i < N; ++i)
                        {
                                for (unsigned j = i + 1; j < N; ++j)
                                {
                                        vertex_set.insert(
                                                to_vector<float>((facet.vertices[i] + facet.vertices[j]).normalized()));
                                }
                        }
                }
                vertices.clear();
                vertices.insert(vertices.cend(), vertex_set.cbegin(), vertex_set.cend());

                std::vector<ConvexHullFacet<N>> ch_facets;
                ProgressRatio progress(nullptr);

                compute_convex_hull(vertices, &ch_facets, &progress);

                for (const ConvexHullFacet<N>& ch_facet : ch_facets)
                {
                        Facet<N, T>& f = tmp.emplace_back();
                        for (unsigned i = 0; i < N; ++i)
                        {
                                int vertex_index = ch_facet.vertices()[i];
                                f.vertices[i] = to_vector<T>(vertices[vertex_index]);
                        }
                }

                subdivision_count = tmp.size() / facets.size();
                facets = std::move(tmp);
        }

        return facets;
}

template <unsigned I, std::size_t N, typename T>
void create_facets(Vector<N, T>* point, std::vector<Facet<N, T>>* facets)
{
        static_assert(I <= N);
        if constexpr (I == N)
        {
                Facet<N, T>& facet = facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        facet.vertices[i] = Vector<N, T>(0);
                        facet.vertices[i][i] = (*point)[i];
                }
        }
        else
        {
                (*point)[I] = -1;
                create_facets<I + 1>(point, facets);
                (*point)[I] = 1;
                create_facets<I + 1>(point, facets);
        }
}

template <std::size_t N, typename T>
std::enable_if_t<N >= 4, std::vector<Facet<N, T>>> create_initial_shape()
{
        // Cross-polytope

        std::vector<Facet<N, T>> facets;
        facets.reserve(power<N>(2));
        Vector<N, T> point;
        create_facets<0>(&point, &facets);
        ASSERT(facets.size() == power<N>(2));
        return facets;
}

template <std::size_t N, typename T>
std::enable_if_t<N == 3, std::vector<Facet<N, T>>> create_initial_shape()
{
        // Regular icosahedron

        const T p = (1 + std::sqrt(T(5))) / 2;

        std::vector<Vector<N, T>> vertices;

        vertices.emplace_back(-1, p, 0);
        vertices.emplace_back(1, p, 0);
        vertices.emplace_back(-1, -p, 0);
        vertices.emplace_back(1, -p, 0);
        vertices.emplace_back(0, -1, p);
        vertices.emplace_back(0, 1, p);
        vertices.emplace_back(0, -1, -p);
        vertices.emplace_back(0, 1, -p);
        vertices.emplace_back(p, 0, -1);
        vertices.emplace_back(p, 0, 1);
        vertices.emplace_back(-p, 0, -1);
        vertices.emplace_back(-p, 0, 1);

        for (Vector<N, T>& v : vertices)
        {
                v.normalize();
        }

        std::vector<Facet<N, T>> facets;

        facets.emplace_back(vertices[0], vertices[1], vertices[7]);
        facets.emplace_back(vertices[0], vertices[5], vertices[1]);
        facets.emplace_back(vertices[0], vertices[7], vertices[10]);
        facets.emplace_back(vertices[0], vertices[10], vertices[11]);
        facets.emplace_back(vertices[0], vertices[11], vertices[5]);
        facets.emplace_back(vertices[1], vertices[5], vertices[9]);
        facets.emplace_back(vertices[2], vertices[4], vertices[11]);
        facets.emplace_back(vertices[3], vertices[2], vertices[6]);
        facets.emplace_back(vertices[3], vertices[4], vertices[2]);
        facets.emplace_back(vertices[3], vertices[6], vertices[8]);
        facets.emplace_back(vertices[3], vertices[8], vertices[9]);
        facets.emplace_back(vertices[3], vertices[9], vertices[4]);
        facets.emplace_back(vertices[4], vertices[9], vertices[5]);
        facets.emplace_back(vertices[5], vertices[11], vertices[4]);
        facets.emplace_back(vertices[6], vertices[2], vertices[10]);
        facets.emplace_back(vertices[7], vertices[1], vertices[8]);
        facets.emplace_back(vertices[8], vertices[6], vertices[7]);
        facets.emplace_back(vertices[9], vertices[8], vertices[1]);
        facets.emplace_back(vertices[10], vertices[7], vertices[6]);
        facets.emplace_back(vertices[11], vertices[10], vertices[2]);

        return facets;
}

template <std::size_t N, typename T>
std::vector<Facet<N, T>> create_sphere(unsigned facet_min_count)
{
        std::vector<Facet<N, T>> facets = create_initial_shape<N, T>();

        for (Facet<N, T>& facet : facets)
        {
                if (!(equal_distances(facet.vertices) && equal_distance_from_origin(facet.vertices)))
                {
                        error("Error creating initial shape");
                }
        }

        return divide_facets(facet_min_count, std::move(facets));
}

template <std::size_t N>
void check_manifold(const std::vector<std::array<int, N>>& facets)
{
        struct Hash
        {
                std::size_t operator()(const std::array<int, N - 1>& v) const
                {
                        return array_hash(v);
                }
        };
        std::unordered_map<std::array<int, N - 1>, int, Hash> ridges;
        for (const std::array<int, N>& facet : facets)
        {
                for (unsigned r = 0; r < N; ++r)
                {
                        std::array<int, N - 1> ridge = sort(del_elem(facet, r));
                        ++ridges[ridge];
                }
        }
        for (const auto& [ridge, count] : ridges)
        {
                if (count != 2)
                {
                        error("Error creating sphere: facet count " + to_string(count) + " is not equal to 2 for ridge "
                              + to_string(ridge));
                }
        }
}

template <std::size_t N, typename T>
void create_mesh(
        const std::vector<Facet<N, T>>& facets,
        std::vector<Vector<N, T>>* mesh_vertices,
        std::vector<std::array<int, N>>* mesh_facets)
{
        mesh_vertices->clear();
        mesh_facets->clear();
        mesh_facets->reserve(facets.size());

        std::unordered_map<Vector<N, T>, unsigned> map;
        map.reserve(N * facets.size());

        for (const Facet<N, T>& facet : facets)
        {
                std::array<int, N>& mesh_facet = mesh_facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = map.find(facet.vertices[i]);
                        if (iter == map.cend())
                        {
                                iter = map.emplace(facet.vertices[i], mesh_vertices->size()).first;
                                mesh_vertices->push_back(facet.vertices[i]);
                        }
                        mesh_facet[i] = iter->second;
                }
        }
}
}

template <std::size_t N, typename T>
void create_sphere(
        unsigned facet_min_count,
        std::vector<Vector<N, T>>* vertices,
        std::vector<std::array<int, N>>* facets)
{
        std::vector<Facet<N, T>> vertex_facets = create_sphere<N, T>(facet_min_count);

        create_mesh(vertex_facets, vertices, facets);

        check_manifold(*facets);

        LOG("Sphere: vertex count = " + to_string(vertices->size()) + ", facet count = " + to_string(facets->size()));
}

template void create_sphere(unsigned, std::vector<Vector<3, float>>*, std::vector<std::array<int, 3>>*);
template void create_sphere(unsigned, std::vector<Vector<4, float>>*, std::vector<std::array<int, 4>>*);
template void create_sphere(unsigned, std::vector<Vector<5, float>>*, std::vector<std::array<int, 5>>*);
template void create_sphere(unsigned, std::vector<Vector<6, float>>*, std::vector<std::array<int, 6>>*);
template void create_sphere(unsigned, std::vector<Vector<3, double>>*, std::vector<std::array<int, 3>>*);
template void create_sphere(unsigned, std::vector<Vector<4, double>>*, std::vector<std::array<int, 4>>*);
template void create_sphere(unsigned, std::vector<Vector<5, double>>*, std::vector<std::array<int, 5>>*);
template void create_sphere(unsigned, std::vector<Vector<6, double>>*, std::vector<std::array<int, 6>>*);
}
