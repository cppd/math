/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "facet.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ns::painter::shapes::mesh
{
namespace
{
template <typename T>
constexpr T MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL = 0.7;

template <std::size_t N, typename T>
[[nodiscard]] std::array<T, N> compute_dots(
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

template <std::size_t N, typename T>
[[nodiscard]] bool all_unidirectional(const std::array<T, N>& dots)
{
        static_assert(MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL<T> > 0);

        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(std::isfinite(dots[i]) && std::abs(dots[i]) >= MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL<T>))
                {
                        return false;
                }
        }
        return true;
}
}

template <std::size_t N, typename T>
void Facet<N, T>::set_texcoords(const bool has_texcoords, const std::array<int, N>& texcoord_indices)
{
        ASSERT((has_texcoords && all_non_negative(texcoord_indices)) || !has_texcoords);

        if (has_texcoords)
        {
                t_ = texcoord_indices;
        }
        else
        {
                t_[0] = -1;
        }
}

template <std::size_t N, typename T>
void Facet<N, T>::set_normals(
        const std::vector<numerical::Vector<N, T>>& normals,
        const bool has_normals,
        const std::array<int, N>& normal_indices)
{
        ASSERT((has_normals && all_non_negative(normal_indices)) || !has_normals);

        if (!has_normals)
        {
                normal_type_ = NormalType::NONE;
                return;
        }

        const std::array<T, N> dots = compute_dots(normals, normal_indices, simplex_.normal());

        if (!all_unidirectional(dots))
        {
                normal_type_ = NormalType::NONE;
                return;
        }

        n_ = normal_indices;

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

template <std::size_t N, typename T>
Facet<N, T>::Facet(
        const std::array<numerical::Vector<N, T>, N>& vertices,
        const std::vector<numerical::Vector<N, T>>& normals,
        const bool has_normals,
        const std::array<int, N>& normal_indices,
        const bool has_texcoords,
        const std::array<int, N>& texcoord_indices,
        const int material)
        : simplex_(vertices),
          material_(material)
{
        set_texcoords(has_texcoords, texcoord_indices);

        set_normals(normals, has_normals, normal_indices);
}

#define TEMPLATE(N, T) template class Facet<N, T>;

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
