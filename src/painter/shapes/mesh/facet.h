/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <cstddef>
#include <vector>

namespace ns::painter::shapes::mesh
{
template <std::size_t N, typename T>
class Facet final
{
        static_assert(N >= 3);

        static constexpr T MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL = 0.7;

        [[nodiscard]] static std::array<T, N> compute_dots(
                const std::vector<numerical::Vector<N, T>>& normals,
                const std::array<int, N>& normal_indices,
                const numerical::Vector<N, T>& simplex_normal)
        {
                std::array<T, N> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = dot(normals[normal_indices[i]], simplex_normal);
                }
                return res;
        }

        [[nodiscard]] static bool all_unidirectional(const std::array<T, N>& dots)
        {
                static_assert(MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL > 0);
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(std::isfinite(dots[i]) && std::abs(dots[i]) >= MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL))
                        {
                                return false;
                        }
                }
                return true;
        }

        enum class NormalType : char
        {
                NONE,
                USE,
                REVERSE
        };

        geometry::spatial::HyperplaneSimplex<N, T> simplex_;
        std::array<int, N> n_;
        std::array<int, N> t_;
        int material_;
        NormalType normal_type_;
        std::array<bool, N> reverse_normal_;

public:
        Facet(const std::array<numerical::Vector<N, T>, N>& vertices,
              const std::vector<numerical::Vector<N, T>>& normals,
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

                const std::array<T, N> dots = compute_dots(normals, normal_indices, simplex_.normal());

                if (!all_unidirectional(dots))
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
                for (std::size_t i = 0; i < N; ++i)
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

        [[nodiscard]] numerical::Vector<N - 1, T> texcoord(
                const std::vector<numerical::Vector<N - 1, T>>& mesh_texcoords,
                const numerical::Vector<N, T>& point) const
        {
                if (has_texcoord())
                {
                        std::array<numerical::Vector<N - 1, T>, N> texcoords;
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                texcoords[i] = mesh_texcoords[t_[i]];
                        }
                        return simplex_.interpolate(point, texcoords);
                }
                error("Mesh facet texture coordinates request when there are no texture coordinates");
        }

        [[nodiscard]] numerical::Vector<N, T> shading_normal(
                const std::vector<numerical::Vector<N, T>>& mesh_normals,
                const numerical::Vector<N, T>& point) const
        {
                switch (normal_type_)
                {
                case NormalType::NONE:
                {
                        return simplex_.normal();
                }
                case NormalType::USE:
                {
                        std::array<numerical::Vector<N, T>, N> normals;
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                normals[i] = mesh_normals[n_[i]];
                        }
                        return simplex_.interpolate(point, normals).normalized();
                }
                case NormalType::REVERSE:
                {
                        std::array<numerical::Vector<N, T>, N> normals;
                        for (std::size_t i = 0; i < N; ++i)
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

        [[nodiscard]] decltype(auto) intersect(const numerical::Ray<N, T>& ray) const
        {
                return simplex_.intersect(ray);
        }

        [[nodiscard]] decltype(auto) geometric_normal() const
        {
                return simplex_.normal();
        }

        [[nodiscard]] decltype(auto) project(const numerical::Vector<N, T>& point) const
        {
                return simplex_.project(point);
        }
};
}
