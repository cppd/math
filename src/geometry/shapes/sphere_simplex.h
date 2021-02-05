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

#include <src/com/constant.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace ns::geometry
{
//template <typename T>
//T sphere_simplex_area(const std::array<Vector<3, T>, 3>& vectors)
//{
//        static_assert(std::is_floating_point_v<T>);
//
//        std::array<Vector<3, T>, 2> v;
//
//        v[0] = vectors[0];
//        v[1] = vectors[1];
//        Vector<3, T> facet_01_normal = numerical::ortho_nn(v);
//        {
//                T norm = facet_01_normal.norm();
//                if (norm == 0)
//                {
//                        return 0;
//                }
//                facet_01_normal /= norm;
//        }
//
//        v[0] = vectors[1];
//        v[1] = vectors[2];
//        Vector<3, T> facet_12_normal = numerical::ortho_nn(v);
//        {
//                T norm = facet_12_normal.norm();
//                if (norm == 0)
//                {
//                        return 0;
//                }
//                facet_12_normal /= norm;
//        }
//
//        v[0] = vectors[2];
//        v[1] = vectors[0];
//        Vector<3, T> facet_20_normal = numerical::ortho_nn(v);
//        {
//                T norm = facet_20_normal.norm();
//                if (norm == 0)
//                {
//                        return 0;
//                }
//                facet_20_normal /= norm;
//        }
//
//        T dihedral_cosine_0 = -dot(facet_01_normal, facet_20_normal);
//        T dihedral_cosine_1 = -dot(facet_01_normal, facet_12_normal);
//        T dihedral_cosine_2 = -dot(facet_20_normal, facet_12_normal);
//
//        T dihedral_0 = std::acos(std::clamp<T>(dihedral_cosine_0, -1, 1));
//        T dihedral_1 = std::acos(std::clamp<T>(dihedral_cosine_1, -1, 1));
//        T dihedral_2 = std::acos(std::clamp<T>(dihedral_cosine_2, -1, 1));
//
//        T area = dihedral_0 + dihedral_1 + dihedral_2 - PI<T>;
//
//        return std::max<T>(0, area);
//}

template <std::size_t N, typename T>
T sphere_simplex_area(const std::array<Vector<N, T>, 2>& vectors)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        T norm_0 = vectors[0].norm();
        if (norm_0 == 0)
        {
                return 0;
        }

        T norm_1 = vectors[1].norm();
        if (norm_1 == 0)
        {
                return 0;
        }

        T cosine = dot(vectors[0] / norm_0, vectors[1] / norm_1);

        return std::acos(std::clamp<T>(cosine, -1, 1));
}

template <std::size_t N, typename T>
T sphere_simplex_area(const std::array<Vector<N, T>, 3>& vectors)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        T norm_0 = vectors[0].norm();
        if (norm_0 == 0)
        {
                return 0;
        }

        T norm_1 = vectors[1].norm();
        if (norm_1 == 0)
        {
                return 0;
        }

        T norm_2 = vectors[2].norm();
        if (norm_2 == 0)
        {
                return 0;
        }

        Vector<N, T> a = vectors[0] / norm_0;
        Vector<N, T> b = vectors[1] / norm_1;
        Vector<N, T> c = vectors[2] / norm_2;

        T cos_a = dot(b, c);
        T cos_b = dot(a, c);
        T cos_c = dot(a, b);

        T sin_a_2 = T(1) - square(cos_a);
        T sin_b_2 = T(1) - square(cos_b);
        T sin_c_2 = T(1) - square(cos_c);

        T dihedral_cosine_a = (cos_a - cos_b * cos_c) / std::sqrt(sin_b_2 * sin_c_2);
        T dihedral_cosine_b = (cos_b - cos_a * cos_c) / std::sqrt(sin_a_2 * sin_c_2);
        T dihedral_cosine_c = (cos_c - cos_a * cos_b) / std::sqrt(sin_a_2 * sin_b_2);

        T dihedral_a = std::acos(std::clamp<T>(dihedral_cosine_a, -1, 1));
        T dihedral_b = std::acos(std::clamp<T>(dihedral_cosine_b, -1, 1));
        T dihedral_c = std::acos(std::clamp<T>(dihedral_cosine_c, -1, 1));

        T area = dihedral_a + dihedral_b + dihedral_c - PI<T>;

        return std::max<T>(0, area);
}
}
