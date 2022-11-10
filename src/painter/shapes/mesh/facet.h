/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <vector>

namespace ns::painter::shapes::mesh
{
template <std::size_t N, typename T>
class Facet final
{
        static_assert(N >= 3);

        static constexpr T MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL = 0.7;

        enum class NormalType : char
        {
                NONE,
                USE,
                REVERSE
        };

        geometry::HyperplaneSimplex<N, T> simplex_;
        std::array<int, N> n_;
        std::array<int, N> t_;
        int material_;
        NormalType normal_type_;
        std::array<bool, N> reverse_normal_;

public:
        Facet(const std::array<Vector<N, T>, N>& vertices,
              const std::vector<Vector<N, T>>& normals,
              const bool has_normals,
              const std::array<int, N>& normal_indices,
              const bool has_texcoords,
              const std::array<int, N>& texcoord_indices,
              const int material)
                : simplex_(vertices),
                  material_(material)
        {
                ASSERT((has_normals && all_non_negative(normal_indices)) || !has_normals);
                ASSERT((has_texcoords && all_non_negative(texcoord_indices)) || !has_texcoords);

                if (has_texcoords)
                {
                        t_ = texcoord_indices;
                }
                else
                {
                        t_[0] = -1;
                }

                if (!has_normals)
                {
                        normal_type_ = NormalType::NONE;
                        return;
                }

                n_ = normal_indices;

                const std::array<T, N> dots = [&]
                {
                        std::array<T, N> res;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                res[i] = dot(normals[n_[i]], simplex_.normal());
                        }
                        return res;
                }();

                if (!std::all_of(
                            dots.cbegin(), dots.cend(),
                            [](const T& d)
                            {
                                    static_assert(MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL > 0);
                                    return std::isfinite(d) && std::abs(d) >= MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL;
                            }))
                {
                        normal_type_ = NormalType::NONE;
                        return;
                }

                if (all_positive(dots))
                {
                        normal_type_ = NormalType::USE;
                        return;
                }

                if (all_negative(dots))
                {
                        normal_type_ = NormalType::USE;
                        simplex_.reverse_normal();
                        return;
                }

                normal_type_ = NormalType::REVERSE;
                for (unsigned i = 0; i < N; ++i)
                {
                        reverse_normal_[i] = dots[i] < 0;
                }
        }

        [[nodiscard]] int material() const
        {
                return material_;
        }

        [[nodiscard]] bool has_texcoord() const
        {
                return t_[0] >= 0;
        }

        [[nodiscard]] Vector<N - 1, T> texcoord(
                const std::vector<Vector<N - 1, T>>& mesh_texcoords,
                const Vector<N, T>& point) const
        {
                if (has_texcoord())
                {
                        std::array<Vector<N - 1, T>, N> texcoords;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                texcoords[i] = mesh_texcoords[t_[i]];
                        }
                        return simplex_.interpolate(point, texcoords);
                }
                error("Mesh facet texture coordinates request when there are no texture coordinates");
        }

        [[nodiscard]] Vector<N, T> shading_normal(
                const std::vector<Vector<N, T>>& mesh_normals,
                const Vector<N, T>& point) const
        {
                switch (normal_type_)
                {
                case NormalType::NONE:
                {
                        return simplex_.normal();
                }
                case NormalType::USE:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = mesh_normals[n_[i]];
                        }
                        return simplex_.interpolate(point, normals).normalized();
                }
                case NormalType::REVERSE:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = reverse_normal_[i] ? -mesh_normals[n_[i]] : mesh_normals[n_[i]];
                        }
                        return simplex_.interpolate(point, normals).normalized();
                }
                }
                error_fatal("Unknown mesh facet normal type " + to_string(enum_to_int(normal_type_)));
        }

        //

        [[nodiscard]] static decltype(auto) intersection_cost()
        {
                return decltype(simplex_)::intersection_cost();
        }

        [[nodiscard]] decltype(auto) intersect(const Ray<N, T>& ray) const
        {
                return simplex_.intersect(ray);
        }

        [[nodiscard]] decltype(auto) geometric_normal() const
        {
                return simplex_.normal();
        }

        [[nodiscard]] decltype(auto) project(const Vector<N, T>& point) const
        {
                return simplex_.project(point);
        }
};
}