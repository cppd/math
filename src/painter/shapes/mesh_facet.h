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

#pragma once

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/geometry/spatial/constraint.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <string>
#include <vector>

namespace ns::painter
{
namespace mesh_facet_implementation
{
template <std::size_t N, typename T>
std::string vertices_to_string(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& v)
{
        std::string res;
        for (unsigned i = 0; i < N; ++i)
        {
                res += to_string(vertices[v[i]]);
                if (i != N - 1)
                {
                        res += '\n';
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> vertices_to_array(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& v)
{
        std::array<Vector<N, T>, N> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = vertices[v[i]];
        }
        return res;
}
}

template <std::size_t N, typename T>
class MeshFacet
{
        static_assert(N >= 3);

        // Минимум абсолютного значения косинуса угла между нормалью симплекса и нормалями
        // его вершин, при котором используются эти нормали вершин. При меньших значениях
        // косинуса нормали вершин считаются неправильными и игнорируются.
        static constexpr T LIMIT_COSINE = 0.7; // 0.7 немного больше 45 градусов

        static constexpr int VERTEX_COUNT = N;

        // Количество сочетаний по 2 из N
        // N! / ((N - 2)! * 2!) = (N * (N - 1)) / 2
        static constexpr int EDGE_COUNT = (N * (N - 1)) / 2;

        const std::vector<Vector<N, T>>& vertices_;
        const std::vector<Vector<N, T>>& normals_;
        const std::vector<Vector<N - 1, T>>& texcoords_;

        std::array<int, N> v_;
        std::array<int, N> n_;
        std::array<int, N> t_;

        int material_;

        Vector<N, T> normal_;

        geometry::HyperplaneSimplex<N, T> geometry_;

        enum class NormalType : char
        {
                None,
                Use,
                Reverse
        } normal_type_;

        std::array<bool, N> reverse_normal_;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        MeshFacet(
                const std::vector<Vector<N, T>>& vertices,
                const std::vector<Vector<N, T>>& normals,
                const std::vector<Vector<N - 1, T>>& texcoords,
                const std::array<int, N>& vertex_indices,
                bool has_normals,
                const std::array<int, N>& normal_indices,
                bool has_texcoords,
                const std::array<int, N>& texcoord_indices,
                int material)
                : vertices_(vertices), normals_(normals), texcoords_(texcoords)
        {
                ASSERT((has_normals && all_non_negative(normal_indices)) || !has_normals);
                ASSERT((has_texcoords && all_non_negative(texcoord_indices)) || !has_texcoords);

                v_ = vertex_indices;

                if (has_texcoords)
                {
                        t_ = texcoord_indices;
                }
                else
                {
                        t_[0] = -1;
                }

                material_ = material;

                normal_ = numerical::orthogonal_complement(vertices_, v_).normalized();

                if (!is_finite(normal_))
                {
                        error("Mesh facet normal is not finite, facet vertices\n"
                              + mesh_facet_implementation::vertices_to_string(vertices_, v_));
                }

                geometry_.set_data(normal_, mesh_facet_implementation::vertices_to_array(vertices_, v_));

                if (!has_normals)
                {
                        normal_type_ = NormalType::None;
                        return;
                }

                n_ = normal_indices;

                std::array<T, N> dots;

                for (unsigned i = 0; i < N; ++i)
                {
                        dots[i] = dot(normals_[n_[i]], normal_);
                }

                if (!std::all_of(
                            dots.cbegin(), dots.cend(),
                            [](const T& d)
                            {
                                    static_assert(LIMIT_COSINE > 0);
                                    return is_finite(d) && std::abs(d) >= LIMIT_COSINE;
                            }))
                {
                        // «Перпендикуляры» на вершинах совсем не перпендикуляры,
                        // поэтому симплекс считать плоским.
                        normal_type_ = NormalType::None;
                        return;
                }

                if (all_positive(dots))
                {
                        // Реальный перпендикуляр и «перпендикуляры» вершин имеют
                        // одинаковое направление, поэтому оставить как есть.
                        normal_type_ = NormalType::Use;
                        return;
                }

                if (all_negative(dots))
                {
                        // Реальный перпендикуляр и все «перпендикуляры» вершин имеют
                        // противоположное направление, поэтому поменять направление
                        // реального перпендикуляра.
                        normal_type_ = NormalType::Use;
                        normal_ = -normal_;
                        return;
                }

                // «Перпендикуляры» на вершинах могут быть направлены в разные стороны от грани.
                // Это происходит, например, при восстановлении поверхностей по алгоритмам
                // типа Cocone, где соседние объекты Вороного имеют положительные полюсы
                // в противоположных направлениях.
                normal_type_ = NormalType::Reverse;
                for (unsigned i = 0; i < N; ++i)
                {
                        reverse_normal_[i] = dots[i] < 0;
                }
        }

        int material() const
        {
                return material_;
        }

        bool has_texcoord() const
        {
                return t_[0] >= 0;
        }

        Vector<N - 1, T> texcoord(const Vector<N, T>& point) const
        {
                if (has_texcoord())
                {
                        std::array<Vector<N - 1, T>, N> texcoords;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                texcoords[i] = texcoords_[t_[i]];
                        }
                        return geometry_.interpolate(point, texcoords);
                }
                error("Mesh facet texture coordinates request when there are no texture coordinates");
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return geometry_.intersect(r, vertices_[v_[0]], normal_);
        }

        Vector<N, T> geometric_normal() const
        {
                return normal_;
        }

        Vector<N, T> shading_normal(const Vector<N, T>& point) const
        {
                switch (normal_type_)
                {
                case NormalType::None:
                {
                        return normal_;
                }
                case NormalType::Use:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = normals_[n_[i]];
                        }
                        return geometry_.interpolate(point, normals).normalized();
                }
                case NormalType::Reverse:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = reverse_normal_[i] ? -normals_[n_[i]] : normals_[n_[i]];
                        }
                        return geometry_.interpolate(point, normals).normalized();
                }
                }
                error_fatal("Unknown mesh facet normal type");
        }

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const
        {
                return mesh_facet_implementation::vertices_to_array(vertices_, v_);
        }

        geometry::Constraints<N, T, N, 1> constraints() const
        {
                return geometry_.constraints(normal_, mesh_facet_implementation::vertices_to_array(vertices_, v_));
        }

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const
        {
                static_assert(N <= 3);

                std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> result;
                unsigned n = 0;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                result[n++] = {vertices_[v_[i]], vertices_[v_[j]] - vertices_[v_[i]]};
                        }
                }
                ASSERT(n == EDGE_COUNT);
                return result;
        }
};
}
