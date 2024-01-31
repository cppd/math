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

#include <src/com/exponent.h>
#include <src/com/type/limit.h>
#include <src/numerical/quadratic.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <optional>

namespace ns::geometry::reconstruction
{
namespace functions_implementation
{
// cos(3 * PI / 8)
template <typename T>
inline constexpr T COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS = 0.38268343236508977172845998403039886676134456248563L;
}

// Check if a Voronoi edge e = (a, b) intersects the cocone of p,
// n is the unit vector of the pole vector vp.
template <typename T>
bool voronoi_edge_intersects_cocone(const T cos_n_pa, const T cos_n_pb)
{
        static_assert(std::is_floating_point_v<T>);
        static constexpr auto COS = functions_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>;

        if ((std::abs(cos_n_pa) < COS) || (std::abs(cos_n_pb) < COS))
        {
                return true;
        }

        if (cos_n_pa < 0 && cos_n_pb > 0)
        {
                return true;
        }

        if (cos_n_pa > 0 && cos_n_pb < 0)
        {
                return true;
        }

        return false;
}

template <typename... T>
bool cocone_inside_or_equal(const T... cos_n_p)
{
        static_assert((std::is_floating_point_v<T> && ...));

        return ((std::abs(cos_n_p) <= functions_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>)&&...);
}

/*
Intersection of a vector and a double cone.

alpha: the opening angle with the axis.
N: the unit cone axis.
PA: the vector from the apex to A.
AB: the vector for the intersection.
PI: the vector from the apex to the intersection
of AB and the cone, PA + t × AB, 0 <= t.

normalize(PA + t × AB)⋅N = ±cos(alpha).
Select PI with a maximum length.

a = PA, ab = AB, n = N:
 ((a + t×ab)/norm(a+t×ab))⋅n = ±cos(alpha),
 ((a⋅n + t×ab⋅n))/norm(a+t×ab) = ±cos(alpha),
 ((a⋅n + t×ab⋅n))²/(a+t×ab)² = cos²(alpha).
 t² × ((n⋅ab)² - cos²(alpha)×(ab)²)
  + t¹ × 2 × ((a⋅n)(n⋅ab) - a⋅ab×cos²(alpha))
  + t⁰ × ((a⋅n)² - a²×cos²(alpha))
  = 0.
*/
template <std::size_t N, typename T>
std::optional<T> intersect_cocone_max_distance(
        const numerical::Vector<N, T>& normalized_cone_axis,
        const numerical::Vector<N, T>& from_apex_to_point_a,
        const numerical::Vector<N, T>& vector_from_point_a)
{
        static_assert(std::is_floating_point_v<T>);
        static constexpr auto COS_SQUARED = square(functions_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>);

        const numerical::Vector<N, T>& vec_a = from_apex_to_point_a;
        const numerical::Vector<N, T>& vec_ab = vector_from_point_a;
        const numerical::Vector<N, T>& vec_norm = normalized_cone_axis;

        const T n_ab = dot(vec_norm, vec_ab);
        const T a_n = dot(vec_a, vec_norm);
        const T square_a = dot(vec_a, vec_a);
        const T square_ab = dot(vec_ab, vec_ab);
        const T a_ab = dot(vec_a, vec_ab);

        // ax² + bx + c = 0
        const T a = square(n_ab) - COS_SQUARED * square_ab;
        const T b = 2 * (a_n * n_ab - a_ab * COS_SQUARED);
        const T c = square(a_n) - square_a * COS_SQUARED;

        T t1;
        T t2;
        if (!numerical::quadratic_equation(a, b, c, &t1, &t2))
        {
                return std::nullopt;
        }

        const bool t1_ok = t1 >= 0 && t1 <= Limits<T>::max();
        const bool t2_ok = t2 >= 0 && t2 <= Limits<T>::max();

        if (!t1_ok && !t2_ok)
        {
                return std::nullopt;
        }
        if (t1_ok && !t2_ok)
        {
                return (vec_a + t1 * vec_ab).norm();
        }
        if (!t1_ok && t2_ok)
        {
                return (vec_a + t2 * vec_ab).norm();
        }

        const T d_1 = (vec_a + t1 * vec_ab).norm_squared();
        const T d_2 = (vec_a + t2 * vec_ab).norm_squared();

        return (d_1 > d_2) ? std::sqrt(d_1) : std::sqrt(d_2);
}
}
